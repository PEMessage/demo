#!/usr/bin/env python3
"""Step 3: introduce the RELATED state.

Companion to the "Linux firewall" lesson, step three.

Pain point from step two:
    A stateful firewall now lets replies of known connections through. But some
    packets are causally tied to a known connection while NOT sharing its
    5-tuple, so conntrack reports NEW and the default policy kills them.

Two real examples:
    1. ICMP errors (destination unreachable, TTL exceeded, ...). Their payload
       embeds the IP header of the original datagram, so we can recover the
       original packet and look up its connection.
    2. FTP data connections. After the control connection to :21 is accepted,
       the data connection is a different 5-tuple. A protocol helper registers
       an "expectation" so future tuples are recognized.

Only one new state is introduced here: RELATED.
The concept behind it:
    A packet may be related to an existing connection even though it is not
    part of it. conntrack produces RELATED in two ways -- by reading an ICMP
    error's embedded packet, and by matching a helper's expectation.

INVALID is deliberately left to a later step.

Run: python3 step3_related.py
"""

from __future__ import annotations

from dataclasses import dataclass
from enum import Enum, auto
from typing import Callable, Dict, List, Optional, Protocol, Tuple


class Verdict(Enum):
    ACCEPT = auto()
    DROP = auto()


class CtState(Enum):
    NEW = auto()
    ESTABLISHED = auto()
    RELATED = auto()


@dataclass(frozen=True)
class Packet:
    src_ip: str
    dst_ip: str
    proto: str
    src_port: Optional[int] = None
    dst_port: Optional[int] = None
    # For an ICMP error: the original datagram echoed inside the payload.
    # This is how the kernel recovers which connection the error belongs to.
    embedded: Optional["Packet"] = None


@dataclass(frozen=True)
class ConnKey:
    proto: str
    ip_a: str
    port_a: Optional[int]
    ip_b: str
    port_b: Optional[int]

    @staticmethod
    def from_packet(pkt: Packet) -> "ConnKey":
        # Canonicalize both directions: A -> B and B -> A yield the same key.
        ep1: Tuple[str, Optional[int]] = (pkt.src_ip, pkt.src_port)
        ep2: Tuple[str, Optional[int]] = (pkt.dst_ip, pkt.dst_port)
        if ep1 <= ep2:
            return ConnKey(pkt.proto, ep1[0], ep1[1], ep2[0], ep2[1])
        return ConnKey(pkt.proto, ep2[0], ep2[1], ep1[0], ep1[1])


@dataclass(frozen=True)
class Expectation:
    """A helper's prediction of a future, related 5-tuple."""

    name: str
    parent: ConnKey
    match: Callable[[Packet], bool]


class Conntrack:
    """Global connection tracking table, shared by every chain."""

    def __init__(self) -> None:
        self.table: Dict[ConnKey, CtState] = {}
        self.expectations: List[Expectation] = []

    def add_expectation(self, exp: Expectation) -> None:
        self.expectations.append(exp)

    def state_of(self, pkt: Packet) -> CtState:
        key = ConnKey.from_packet(pkt)
        if key in self.table:
            return CtState.ESTABLISHED

        # RELATED (way 1): an ICMP error that quotes a known connection.
        if pkt.embedded is not None:
            parent = ConnKey.from_packet(pkt.embedded)
            if parent in self.table:
                return CtState.RELATED

        # RELATED (way 2): a tuple predicted by a helper's expectation,
        # as long as the parent connection still exists.
        for exp in self.expectations:
            if exp.parent in self.table and exp.match(pkt):
                return CtState.RELATED

        return CtState.NEW

    def commit(self, pkt: Packet, verdict: Verdict) -> None:
        if verdict is not Verdict.ACCEPT:
            return
        # Only real transport connections are tracked. An ICMP error is a
        # verdict on someone else's connection, not a connection of its own.
        if pkt.proto not in ("tcp", "udp"):
            return
        self.table[ConnKey.from_packet(pkt)] = CtState.ESTABLISHED


class Helper(Protocol):
    """Stands in for nf_conntrack_<proto>: reacts to accepted packets."""

    def on_accept(self, pkt: Packet, ct: Conntrack) -> None: ...


class FtpHelper:
    """Tiny stand-in for nf_conntrack_ftp.

    On an accepted FTP control connection, install an expectation for the
    active-mode data connection: server:20 -> client:<random high port>.
    """

    def on_accept(self, pkt: Packet, ct: Conntrack) -> None:
        if pkt.proto == "tcp" and pkt.dst_port == 21:
            server_ip = pkt.dst_ip
            client_ip = pkt.src_ip
            ct.add_expectation(
                Expectation(
                    name="ftp-data",
                    parent=ConnKey.from_packet(pkt),
                    match=lambda p: (
                        p.proto == "tcp"
                        and p.src_ip == server_ip
                        and p.src_port == 20
                        and p.dst_ip == client_ip
                    ),
                )
            )


@dataclass(frozen=True)
class Rule:
    name: str
    match: Callable[[Packet, CtState], bool]
    verdict: Verdict


class Firewall:
    """One decision point (think: one chain), sharing the global Conntrack."""

    def __init__(
        self,
        rules: List[Rule],
        default: Verdict,
        conntrack: Conntrack,
        helpers: Optional[List[Helper]] = None,
    ) -> None:
        self.rules = rules
        self.default = default
        self.conntrack = conntrack
        self.helpers = helpers or []

    def decide(self, pkt: Packet) -> Verdict:
        ct_state = self.conntrack.state_of(pkt)

        for rule in self.rules:
            if rule.match(pkt, ct_state):
                verdict = rule.verdict
                self.conntrack.commit(pkt, verdict)
                if verdict is Verdict.ACCEPT:
                    for helper in self.helpers:
                        helper.on_accept(pkt, self.conntrack)
                return verdict

        verdict = self.default
        self.conntrack.commit(pkt, verdict)
        return verdict


def describe(pkt: Packet) -> str:
    port = f":{pkt.dst_port}" if pkt.dst_port is not None else ""
    text = f"{pkt.src_ip} -> {pkt.dst_ip} [{pkt.proto}{port}]"
    if pkt.embedded is not None:
        text += f" (quotes {pkt.embedded.src_ip}->{pkt.embedded.dst_ip})"
    return text


def main() -> None:
    ct = Conntrack()

    # OUTPUT: the FTP control connection is initiated here, so the helper
    # watches this chain to install the data-connection expectation.
    out_fw = Firewall(
        rules=[
            Rule("allow_established", lambda p, s: s == CtState.ESTABLISHED, Verdict.ACCEPT),
            Rule("allow_new_out", lambda p, s: s == CtState.NEW, Verdict.ACCEPT),
        ],
        default=Verdict.DROP,
        conntrack=ct,
        helpers=[FtpHelper()],
    )

    # INPUT: established replies, RELATED packets, new SSH.
    # Note there is no blanket "allow icmp" rule -- an ICMP error only passes
    # if conntrack classifies it as RELATED.
    in_fw = Firewall(
        rules=[
            Rule("allow_established", lambda p, s: s == CtState.ESTABLISHED, Verdict.ACCEPT),
            Rule("allow_related", lambda p, s: s == CtState.RELATED, Verdict.ACCEPT),
            Rule(
                "allow_new_ssh",
                lambda p, s: s == CtState.NEW and p.proto == "tcp" and p.dst_port == 22,
                Verdict.ACCEPT,
            ),
        ],
        default=Verdict.DROP,
        conntrack=ct,
    )

    local = "192.168.1.10"
    peer = "93.184.216.34"

    def step(title: str, fw: Firewall, pkt: Packet) -> None:
        state = ct.state_of(pkt)
        verdict = fw.decide(pkt)
        print(f"{title:<20} {describe(pkt):<58} ct={state.name:<12} => {verdict.name}")

    print("=== A. ICMP error related to an established connection ===\n")

    # 1) Host starts outbound HTTP; conntrack records it.
    step("outbound", out_fw, Packet(local, peer, "tcp", 54321, 80))
    # 2) Router replies "destination unreachable", quoting our datagram.
    step(
        "icmp error",
        in_fw,
        Packet("10.0.0.1", local, "icmp", embedded=Packet(local, peer, "tcp", 54321, 80)),
    )
    # 3) An ICMP error quoting an unknown connection: not RELATED -> dropped.
    step(
        "icmp unknown",
        in_fw,
        Packet("10.0.0.1", local, "icmp", embedded=Packet(local, "198.51.100.9", "tcp", 60000, 443)),
    )

    print("\n=== B. FTP data connection related via a helper expectation ===\n")

    # 4) Host opens the FTP control connection to server:21 (outbound).
    #    Accepted on OUTPUT; the helper installs an expectation for the
    #    active-mode data connection server:20 -> host:<random>.
    step("ftp control", out_fw, Packet(local, peer, "tcp", 54321, 21))
    # 5) Active-mode data connection from server:20 -- a different 5-tuple,
    #    but RELATED thanks to the helper's expectation.
    step("ftp data", in_fw, Packet(peer, local, "tcp", 20, 54322))
    # 6) Later packets of the data connection are ESTABLISHED now.
    step("ftp data more", in_fw, Packet(peer, local, "tcp", 20, 54322))
    # 7) A data-looking connection from a different server: no expectation -> NEW -> DROP.
    step("ftp foreign", in_fw, Packet("198.51.100.9", local, "tcp", 20, 54323))

    print(f"\nconntrack holds {len(ct.table)} connections, {len(ct.expectations)} expectation(s).")
    print("Result: ICMP errors and FTP data connections tied to known connections")
    print("pass as RELATED, while unrelated packets still hit the default DROP.")
    print("\nNext pain point: packets that look malformed or out of any state, e.g.")
    print("a stray ACK with no connection. conntrack calls these INVALID.")


if __name__ == "__main__":
    main()
