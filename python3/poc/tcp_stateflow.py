"""TCP connection state machine, modelled with Python generators.

Each generator yields its current state name and receives events such as
("recv", "ACK") or ("app", "close"). A connection only knows its own transmit
wire (an outgoing queue); the Sim moves packets off that wire and turns them
into "recv" events on the far end.

The packet sender is injected into each state machine (see ``send`` below),
so nothing the machine touches is global.
"""

from collections import deque


# ---------- Active close path ----------

def fin_wait_1(send):
    event = yield "FIN_WAIT_1"
    if event == ("recv", "ACK"):
        yield from fin_wait_2(send)
    elif event == ("recv", "FIN"):       # simultaneous close
        send("ACK")
        yield from closing(send)
    elif event == ("recv", "RST"):
        return

def fin_wait_2(send):
    event = yield "FIN_WAIT_2"
    if event == ("recv", "FIN"):
        send("ACK")
        yield from time_wait(send)

def closing(send):
    event = yield "CLOSING"
    if event == ("recv", "ACK"):
        yield from time_wait(send)

def time_wait(send):
    event = yield "TIME_WAIT"
    if event == ("timer", "2MSL"):
        return


# ---------- Passive close path ----------

def close_wait(send):
    event = yield "CLOSE_WAIT"
    if event == ("app", "close"):
        send("FIN")
        yield from last_ack(send)

def last_ack(send):
    event = yield "LAST_ACK"
    if event == ("recv", "ACK"):
        return


# ---------- Data phase ----------

def established(send):
    while True:
        event = yield "ESTABLISHED"
        if event == ("app", "close"):
            send("FIN")
            yield from fin_wait_1(send)
            return
        elif event == ("recv", "FIN"):
            send("ACK")
            yield from close_wait(send)
            return
        elif event == ("recv", "RST"):
            return
        # any other event (DATA) keeps us in ESTABLISHED


# ---------- Client path ----------

def client(send):
    send("SYN")
    event = yield "SYN_SENT"
    if event == ("recv", "SYN+ACK"):
        send("ACK")
        yield from established(send)
    elif event == ("recv", "RST"):
        return


# ---------- Server path ----------

def server(send):
    event = yield "LISTEN"
    if event == ("recv", "SYN"):
        send("SYN+ACK")
        event = yield "SYN_RCVD"
        if event == ("recv", "ACK"):
            yield from established(send)
        elif event == ("recv", "RST"):
            return


# ---------- Scheduler ----------

class TCPConn:
    def __init__(self, machine, name="conn"):
        self.name = name
        self.state = None                # set by start()
        self.out = deque()               # transmit wire: packets this end sends
        self.coro = machine(self._send)  # inject this connection's sender

    def _send(self, pkt):
        """Bound sender handed to the state machine; no globals involved."""
        print(f"    TX -> {pkt}")
        self.out.append(pkt)

    def start(self):
        """Run to the first yield and grab the initial state name."""
        self.state = next(self.coro)

    def on(self, event):
        print(f"  [{self.name}] EV <- {event}")
        try:
            self.state = self.coro.send(event)
        except StopIteration:
            self.state = "CLOSED"
        print(f"  [{self.name}]     state = {self.state}")


# ---------- Network + driver ----------

class Sim:
    """Connect two endpoints: move packets off each transmit wire.

    A packet leaving one endpoint becomes a ("recv", ...) event on the other.
    The endpoints stay unaware of each other; the Sim knows the topology.
    """

    def __init__(self, client, server):
        self.client, self.server = client, server
        client.start()                   # client emits SYN onto its wire
        server.start()                   # server waits in LISTEN

    def drain(self):
        """Deliver packets in both directions until both wires are empty."""
        while self.client.out or self.server.out:
            if self.client.out:          # client -> server first, for deterministic output
                pkt = self.client.out.popleft()
                print(f"  [client] -- {pkt} --> [server]")
                self.server.on(("recv", pkt))
            else:
                pkt = self.server.out.popleft()
                print(f"  [server] -- {pkt} --> [client]")
                self.client.on(("recv", pkt))

    def inject(self, conn, event):
        """Inject a non-network event (application close / timer)."""
        conn.on(event)

    def data(self, sender, payload="DATA"):
        """Sender pushes a chunk of application data onto its transmit wire."""
        sender.out.append(payload)


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
    sim.inject(c, ("app", "close"))      # client application closes first
    sim.drain()                          # FIN -> ACK -> FIN -> ACK flows by itself
    sim.inject(s, ("app", "close"))      # server application closes afterwards
    sim.drain()
    sim.inject(c, ("timer", "2MSL"))     # wait out 2MSL, then finish
    sim.drain()
    show_done(c, s)


def scenario_server_close():
    """The server closes first, so the client goes through the passive path."""
    banner("Server-initiated close")
    c, s = make_pair()
    sim = Sim(c, s)
    sim.drain()
    sim.inject(s, ("app", "close"))
    sim.drain()
    sim.inject(c, ("app", "close"))
    sim.drain()
    sim.inject(s, ("timer", "2MSL"))
    sim.drain()
    show_done(c, s)


def scenario_simultaneous_close():
    """Both ends send FIN before receiving the peer's ACK, passing through CLOSING."""
    banner("Simultaneous close")
    c, s = make_pair()
    sim = Sim(c, s)
    sim.drain()
    sim.inject(c, ("app", "close"))
    sim.inject(s, ("app", "close"))      # the two FINs cross on the wire
    sim.drain()
    sim.inject(c, ("timer", "2MSL"))
    sim.inject(s, ("timer", "2MSL"))
    sim.drain()
    show_done(c, s)


def scenario_data_then_close():
    """Data transfer keeps both ends in ESTABLISHED, then they close normally."""
    banner("Data transfer, then close")
    c, s = make_pair()
    sim = Sim(c, s)
    sim.drain()
    sim.data(c, "hello")                 # travels the client transmit wire
    sim.data(s, "world")                 # and the server one
    sim.drain()                          # both ends stay in ESTABLISHED
    sim.inject(c, ("app", "close"))
    sim.drain()
    sim.inject(s, ("app", "close"))
    sim.drain()
    sim.inject(c, ("timer", "2MSL"))
    sim.drain()
    show_done(c, s)


if __name__ == "__main__":
    scenario_client_close()
    scenario_server_close()
    scenario_simultaneous_close()
    scenario_data_then_close()
