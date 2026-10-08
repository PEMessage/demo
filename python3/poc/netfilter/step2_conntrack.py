#!/usr/bin/env python3
"""Step 2: introduce connection tracking (conntrack), making the firewall stateful.

Companion to the "Linux firewall" lesson, step two.

Pain point from step one:
    With INPUT default DROP, when the host initiates an outbound connection the
    peer's reply arrives with a random high destination port. No rule matches,
    so it is dropped and the TCP handshake fails.

Only one core concept is introduced here:
    The conntrack table -- it records a connection's bidirectional 5-tuple
    and its state. States are only NEW / ESTABLISHED here; RELATED / INVALID
    come in the next step.

Key boundaries:
    - 5-tuple: protocol + two (ip, port) endpoints.
    - Bidirectional: A -> B and B -> A are the same connection (same key).
    - conntrack is global: INPUT / OUTPUT / FORWARD share one table.
    - The first accepted packet is committed to the table; later packets of
      the same connection hit the table and become ESTABLISHED.

Run: python3 step2_conntrack.py
"""

from __future__ import annotations

from dataclasses import dataclass
from enum import Enum, auto
from typing import Callable, Dict, List, Optional, Tuple


class Verdict(Enum):
    ACCEPT = auto()
    DROP = auto()


class CtState(Enum):
    NEW = auto()
    ESTABLISHED = auto()


@dataclass(frozen=True)
class Packet:
    """Now includes the source port: a 5-tuple needs it to identify a connection."""

    src_ip: str
    dst_ip: str
    proto: str
    src_port: Optional[int] = None
    dst_port: Optional[int] = None


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


class Conntrack:
    """Global connection tracking table, shared by every chain."""

    def __init__(self) -> None:
        self.table: Dict[ConnKey, CtState] = {}

    def state_of(self, pkt: Packet) -> CtState:
        key = ConnKey.from_packet(pkt)
        if key in self.table:
            return CtState.ESTABLISHED
        return CtState.NEW

    def commit(self, pkt: Packet, verdict: Verdict) -> None:
        if verdict is not Verdict.ACCEPT:
            return
        # Simplified: real conntrack runs a TCP state machine
        # (NEW -> ESTABLISHED). Here an accepted connection is marked
        # ESTABLISHED at once, to capture the essence: replies can hit the table.
        self.table[ConnKey.from_packet(pkt)] = CtState.ESTABLISHED


@dataclass(frozen=True)
class Rule:
    # A rule now sees both the packet and the connection state.
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
    ) -> None:
        self.rules = rules
        self.default = default
        self.conntrack = conntrack

    def decide(self, pkt: Packet) -> Verdict:
        ct_state = self.conntrack.state_of(pkt)

        for rule in self.rules:
            if rule.match(pkt, ct_state):
                verdict = rule.verdict
                self.conntrack.commit(pkt, verdict)
                return verdict

        verdict = self.default
        self.conntrack.commit(pkt, verdict)
        return verdict


def describe(pkt: Packet) -> str:
    return (
        f"{pkt.src_ip}:{pkt.src_port} -> "
        f"{pkt.dst_ip}:{pkt.dst_port} [{pkt.proto}]"
    )


def main() -> None:
    ct = Conntrack()

    # OUTPUT chain: allow packets of established connections and let the
    # host initiate new ones.
    out_fw = Firewall(
        rules=[
            Rule(
                "allow_established",
                lambda p, s: s == CtState.ESTABLISHED,
                Verdict.ACCEPT,
            ),
            Rule("allow_new_out", lambda p, s: s == CtState.NEW, Verdict.ACCEPT),
        ],
        default=Verdict.DROP,
        conntrack=ct,
    )

    # INPUT chain: allow replies of established connections plus new SSH.
    in_fw = Firewall(
        rules=[
            Rule(
                "allow_established",
                lambda p, s: s == CtState.ESTABLISHED,
                Verdict.ACCEPT,
            ),
            Rule(
                "allow_new_ssh",
                lambda p, s: s == CtState.NEW
                and p.proto == "tcp"
                and p.dst_port == 22,
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
        print(f"{title:<22} {describe(pkt):<52} ct={state.name:<12} => {verdict.name}")

    print("=== Scenario: host initiates outbound HTTP, then receives the reply ===\n")

    # 1) Host starts an outbound connection: random source port 54321.
    step("outbound (SYN)", out_fw, Packet(local, peer, "tcp", 54321, 80))

    # 2) Peer's reply: destination port is exactly that 54321.
    #    Key point: the bidirectional ConnKey lets it hit conntrack.
    step("peer reply", in_fw, Packet(peer, local, "tcp", 80, 54321))

    # 3) Later packets of the same connection also hit ESTABLISHED.
    step("later data", out_fw, Packet(local, peer, "tcp", 54321, 80))

    print("\n=== Contrast: a 'new connection' not in the table is still dropped ===\n")

    # 4) An unsolicited inbound connection to port 80 -- NEW, so dropped.
    step("unknown new:80", in_fw, Packet(peer, local, "tcp", 40000, 80))

    # 5) An unsolicited inbound SSH connection -- explicitly allowed.
    step("new SSH", in_fw, Packet(peer, local, "tcp", 40001, 22))

    print(f"\nconntrack currently holds {len(ct.table)} connection entries.")
    print("Result: default DROP no longer kills replies, while unknown new")
    print("connections are still blocked.")
    print("\nNext pain point: FTP data connections / ICMP errors are related to an")
    print("existing connection but are not the same 5-tuple -- that needs RELATED.")


if __name__ == "__main__":
    main()
