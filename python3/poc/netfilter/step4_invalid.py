#!/usr/bin/env python3
"""Step 4: introduce the INVALID state.

Companion to the "Linux firewall" lesson, step four.

Pain point from step three:
    conntrack can now say NEW / ESTABLISHED / RELATED. But some packets fit
    none of them: they are not part of a tracked connection, and they are not
    a connection opener either. A classic case is a stray TCP ACK with no
    handshake behind it.

The new state is INVALID:
    A packet that conntrack cannot classify as part of any connection and that
    is not a legitimate opener. conntrack has no place for it.

Why this matters:
    If any rule matches on fields only (say, destination port 22) without
    looking at the state, an INVALID packet can slip through. Best practice is
    therefore to DROP INVALID explicitly, and to put that rule EARLY.

This file keeps the machinery from earlier steps (NEW / ESTABLISHED / RELATED)
so it runs on its own. FTP expectations are omitted here for brevity; the new
concept is only INVALID.

Run: python3 step4_invalid.py
"""

from __future__ import annotations

from dataclasses import dataclass
from enum import Enum, auto
from typing import Callable, Dict, FrozenSet, List, Optional, Tuple


class Verdict(Enum):
    ACCEPT = auto()
    DROP = auto()


class CtState(Enum):
    NEW = auto()
    ESTABLISHED = auto()
    RELATED = auto()
    INVALID = auto()


@dataclass(frozen=True)
class Packet:
    src_ip: str
    dst_ip: str
    proto: str
    src_port: Optional[int] = None
    dst_port: Optional[int] = None
    # TCP control bits, e.g. frozenset({"SYN"}), frozenset({"SYN", "ACK"}).
    # Needed to tell a connection opener from a stray packet.
    tcp_flags: FrozenSet[str] = frozenset()
    # For an ICMP error: the original datagram echoed inside the payload.
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


def is_connection_opener(pkt: Packet) -> bool:
    """A TCP connection starts with SYN and without ACK. Everything else on
    TCP is only meaningful if it belongs to a tracked connection."""
    return pkt.proto == "tcp" and "SYN" in pkt.tcp_flags and "ACK" not in pkt.tcp_flags


class Conntrack:
    """Global connection tracking table, shared by every chain."""

    def __init__(self) -> None:
        self.table: Dict[ConnKey, CtState] = {}

    def state_of(self, pkt: Packet) -> CtState:
        key = ConnKey.from_packet(pkt)
        if key in self.table:
            return CtState.ESTABLISHED

        # RELATED: an ICMP error that quotes a known connection.
        if pkt.embedded is not None:
            parent = ConnKey.from_packet(pkt.embedded)
            if parent in self.table:
                return CtState.RELATED

        # Not tracked. Is it a legitimate opener, or something out of place?
        if pkt.proto == "tcp":
            return CtState.NEW if is_connection_opener(pkt) else CtState.INVALID
        # UDP / ICMP echo have no handshake: a first packet is simply NEW.
        return CtState.NEW

    def commit(self, pkt: Packet, verdict: Verdict) -> None:
        if verdict is not Verdict.ACCEPT:
            return
        if pkt.proto not in ("tcp", "udp"):
            return
        self.table[ConnKey.from_packet(pkt)] = CtState.ESTABLISHED


@dataclass(frozen=True)
class Rule:
    name: str
    match: Callable[[Packet, CtState], bool]
    verdict: Verdict


class Firewall:
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
    port = f":{pkt.dst_port}" if pkt.dst_port is not None else ""
    flags = f" [{','.join(sorted(pkt.tcp_flags))}]" if pkt.tcp_flags else ""
    text = f"{pkt.src_ip} -> {pkt.dst_ip} [{pkt.proto}{port}]{flags}"
    if pkt.embedded is not None:
        text += f" (quotes {pkt.embedded.src_ip}->{pkt.embedded.dst_ip})"
    return text


def main() -> None:
    ct = Conntrack()

    out_fw = Firewall(
        rules=[
            Rule("allow_established", lambda p, s: s == CtState.ESTABLISHED, Verdict.ACCEPT),
            Rule("allow_new_out", lambda p, s: s == CtState.NEW, Verdict.ACCEPT),
        ],
        default=Verdict.DROP,
        conntrack=ct,
    )

    # Correct INPUT chain: drop INVALID first, then allow known/related/new SSH.
    in_fw = Firewall(
        rules=[
            Rule("drop_invalid", lambda p, s: s == CtState.INVALID, Verdict.DROP),
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

    # A naive INPUT chain: a port-only rule, no explicit INVALID handling.
    naive_in_fw = Firewall(
        rules=[
            Rule("allow_established", lambda p, s: s == CtState.ESTABLISHED, Verdict.ACCEPT),
            Rule("allow_related", lambda p, s: s == CtState.RELATED, Verdict.ACCEPT),
            Rule("allow_dst_22", lambda p, s: p.dst_port == 22, Verdict.ACCEPT),
        ],
        default=Verdict.DROP,
        conntrack=ct,
    )

    local = "192.168.1.10"
    peer = "93.184.216.34"

    def step(title: str, fw: Firewall, pkt: Packet) -> None:
        state = ct.state_of(pkt)
        verdict = fw.decide(pkt)
        print(f"{title:<18} {describe(pkt):<62} ct={state.name:<12} => {verdict.name}")

    print("=== Establish a normal connection, for reference ===\n")
    step("outbound SYN", out_fw, Packet(local, peer, "tcp", 54321, 80, frozenset({"SYN"})))
    step("peer reply", in_fw, Packet(peer, local, "tcp", 80, 54321, frozenset({"SYN", "ACK"})))
    step(
        "icmp error",
        in_fw,
        Packet("10.0.0.1", local, "icmp", embedded=Packet(local, peer, "tcp", 54321, 80)),
    )

    print("\n=== Correct chain: INVALID is dropped early ===\n")
    # A stray ACK to port 22 with no connection behind it.
    step("stray ACK:22", in_fw, Packet(peer, local, "tcp", 50000, 22, frozenset({"ACK"})))
    # A stray RST, also untracked.
    step("stray RST", in_fw, Packet(peer, local, "tcp", 50001, 22, frozenset({"RST"})))
    # A legitimate new SSH connection: SYN only -> NEW -> allowed.
    step("new SYN:22", in_fw, Packet(peer, local, "tcp", 50002, 22, frozenset({"SYN"})))

    print("\n=== Naive chain: a port-only rule lets INVALID slip through ===\n")
    step("stray ACK:22", naive_in_fw, Packet(peer, local, "tcp", 50003, 22, frozenset({"ACK"})))

    print(f"\nconntrack holds {len(ct.table)} connections.")
    print("Result: with an early 'drop INVALID' rule, a stray ACK/RST to port 22")
    print("is rejected, while a real SYN still opens the connection. A port-only")
    print("rule without state handling would have accepted the stray ACK.")
    print("\nNext pain point: this model only looks at one packet at one point.")
    print("Real Linux fires rules at several hook points (PREROUTING/INPUT/")
    print("FORWARD/OUTPUT/POSTROUTING), which is the 'chain' concept.")


if __name__ == "__main__":
    main()
