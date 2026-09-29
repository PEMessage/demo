"""TCP connection state machine, modelled with Python generators.

Each generator is a pure state transducer: it *yields* intents and *receives*
events. Nothing is globals and no I/O happens inside the machine.

Values the machine yields (outbound):
    ("state", name)              current state, driver waits for an event
    ("send", seg)                request to transmit one segment
    ("app", "recv", data)        deliver received data up to the application

Events fed into the machine (inbound):
    ("recv", seg)                a segment arrived from the network
    ("app", "send", data)        application wants to push data
    ("app", "close")             application wants to close
    ("timer", "2MSL")            a timer expired

A segment is a control name ("SYN", "ACK", "FIN", "RST", "SYN+ACK") or a data
tuple ("DATA", payload).
"""

from collections import deque


def send(seg):
    """Yield a transmit request; the driver decides what to do with it.

    Written as a generator so state machines can say `yield from send("ACK")`
    and stay side-effect free.
    """
    yield ("send", seg)


# ---------- Active close path ----------

def fin_wait_1():
    event = yield ("state", "FIN_WAIT_1")
    if event == ("recv", "ACK"):
        yield from fin_wait_2()
    elif event == ("recv", "FIN"):       # simultaneous close
        yield from send("ACK")
        yield from closing()
    elif event == ("recv", "RST"):
        return

def fin_wait_2():
    event = yield ("state", "FIN_WAIT_2")
    if event == ("recv", "FIN"):
        yield from send("ACK")
        yield from time_wait()

def closing():
    event = yield ("state", "CLOSING")
    if event == ("recv", "ACK"):
        yield from time_wait()

def time_wait():
    event = yield ("state", "TIME_WAIT")
    if event == ("timer", "2MSL"):
        return


# ---------- Passive close path ----------

def close_wait():
    event = yield ("state", "CLOSE_WAIT")
    if event == ("app", "close"):
        yield from send("FIN")
        yield from last_ack()

def last_ack():
    event = yield ("state", "LAST_ACK")
    if event == ("recv", "ACK"):
        return


# ---------- Data phase ----------

def established():
    while True:
        event = yield ("state", "ESTABLISHED")
        tag = event[0]
        if tag == "app":
            if event[1] == "close":
                yield from send("FIN")
                yield from fin_wait_1()
                return
            elif event[1] == "send":
                yield from send(("DATA", event[2]))   # naive: one segment per write
                continue
        elif tag == "recv":
            seg = event[1]
            if seg == "FIN":
                yield from send("ACK")
                yield from close_wait()
                return
            elif seg == "RST":
                return
            elif isinstance(seg, tuple) and seg[0] == "DATA":
                yield ("app", "recv", seg[1])          # deliver up, stay ESTABLISHED
                continue
        # anything else is ignored; stay in ESTABLISHED


# ---------- Client path ----------

def client():
    yield from send("SYN")
    event = yield ("state", "SYN_SENT")
    if event == ("recv", "SYN+ACK"):
        yield from send("ACK")
        yield from established()
    elif event == ("recv", "RST"):
        return


# ---------- Server path ----------

def server():
    event = yield ("state", "LISTEN")
    if event == ("recv", "SYN"):
        yield from send("SYN+ACK")
        event = yield ("state", "SYN_RCVD")
        if event == ("recv", "ACK"):
            yield from established()
        elif event == ("recv", "RST"):
            return


# ---------- Scheduler ----------

class TCPConn:
    """Drives one state machine and performs its outbound actions."""

    def __init__(self, machine, name="conn"):
        self.name = name
        self.state = None                # set by start()
        self.out = deque()               # transmit wire: segments this end sends
        self.coro = machine()

    def start(self):
        """Run to the first ("state", ...) yield."""
        self._pump(None)

    def on(self, event):
        """Feed one inbound event and pump until the machine waits again."""
        if self.state == "CLOSED":
            return
        print(f"  [{self.name}] EV <- {event}")
        self._pump(event)

    def _pump(self, event):
        # One inbound event can yield several actions before the machine parks
        # on its next state, so keep resuming until we see ("state", ...).
        while True:
            try:
                out = self.coro.send(event)
            except StopIteration:
                self.state = "CLOSED"
                print(f"  [{self.name}]     state = {self.state}")
                return
            event = None                     # later sends just resume pending actions
            tag = out[0]
            if tag == "state":
                self.state = out[1]
                print(f"  [{self.name}]     state = {self.state}")
                return
            elif tag == "send":
                self.out.append(out[1])
            elif tag == "app":               # outbound notification to the app
                print(f"  [{self.name}] APP <- {out[1:]}")


# ---------- Network + driver ----------

class Sim:
    """Connect two endpoints: move segments off each transmit wire.

    A segment leaving one endpoint becomes a ("recv", seg) event on the other.
    The endpoints stay unaware of each other; the Sim knows the topology.
    """

    def __init__(self, client, server):
        self.client, self.server = client, server
        client.start()                   # client emits SYN onto its wire
        server.start()                   # server waits in LISTEN

    def drain(self):
        """Deliver segments in both directions until both wires are empty."""
        while self.client.out or self.server.out:
            if self.client.out:          # client -> server first, for deterministic output
                seg = self.client.out.popleft()
                print(f"  [client] -- {seg} --> [server]")
                self.server.on(("recv", seg))
            else:
                seg = self.server.out.popleft()
                print(f"  [server] -- {seg} --> [client]")
                self.client.on(("recv", seg))


def make_pair():
    """Build one client and one server connection (not started yet)."""
    return TCPConn(client, "client"), TCPConn(server, "server")


def show_done(*conns):
    for c in conns:
        mark = "OK" if c.state == "CLOSED" else f"still in {c.state}"
        print(f"  [{c.name}] final: {mark}")


def banner(title):
    print(f"\n=== {title} ===")


# ---------- Scenarios ----------

def scenario_client_close():
    """Three-way handshake, then the client actively closes (classic four-way)."""
    banner("Client-initiated close (four-way handshake)")
    c, s = make_pair()
    sim = Sim(c, s)                      # starts both ends
    sim.drain()                          # complete the three-way handshake
    c.on(("app", "close"))      # client application closes first
    sim.drain()                          # FIN -> ACK -> FIN -> ACK flows by itself
    s.on(("app", "close"))      # server application closes afterwards
    sim.drain()
    c.on(("timer", "2MSL"))     # wait out 2MSL, then finish
    sim.drain()
    show_done(c, s)


def scenario_server_close():
    """The server closes first, so the client goes through the passive path."""
    banner("Server-initiated close")
    c, s = make_pair()
    sim = Sim(c, s)
    sim.drain()
    s.on(("app", "close"))
    sim.drain()
    c.on(("app", "close"))
    sim.drain()
    s.on(("timer", "2MSL"))
    sim.drain()
    show_done(c, s)


def scenario_simultaneous_close():
    """Both ends send FIN before receiving the peer's ACK, passing through CLOSING."""
    banner("Simultaneous close")
    c, s = make_pair()
    sim = Sim(c, s)
    sim.drain()
    c.on(("app", "close"))
    s.on(("app", "close"))      # the two FINs cross on the wire
    sim.drain()
    c.on(("timer", "2MSL"))
    s.on(("timer", "2MSL"))
    sim.drain()
    show_done(c, s)


def scenario_data_then_close():
    """Application data goes up and down, then the connection closes normally."""
    banner("Data transfer, then close")
    c, s = make_pair()
    sim = Sim(c, s)
    sim.drain()                          # three-way handshake
    c.on(("app", "send", "hello"))   # app write -> DATA segment
    s.on(("app", "send", "world"))
    sim.drain()                          # delivered as ("app", "recv", ...) on the far end
    c.on(("app", "close"))
    sim.drain()
    s.on(("app", "close"))
    sim.drain()
    c.on(("timer", "2MSL"))
    sim.drain()
    show_done(c, s)


if __name__ == "__main__":
    scenario_client_close()
    scenario_server_close()
    scenario_simultaneous_close()
    scenario_data_then_close()
