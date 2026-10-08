#!/usr/bin/env python3
"""Step 5: introduce hook points and chains.

Companion to the "Linux firewall" lesson, step five.

Pain point from step four:
    Every previous step assumed a single decision point. Real Linux gives the
    firewall several chances to inspect a packet as it moves through the stack.
    A packet can be dropped at any of them.

The new concept is the hook point (chain):
    Rules are grouped by WHERE in the stack they run. The path a packet takes
    depends on routing:
        - delivered to the local host:  PREROUTING -> INPUT
        - forwarded to another host:    PREROUTING -> FORWARD -> POSTROUTING
        - generated locally:            OUTPUT -> POSTROUTING

Engineering detail that surfaces here:
    The conntrack state of a packet is computed ONCE when it enters, and every
    hook sees the same state. Otherwise a packet accepted at the first hook
    would look ESTABLISHED at the next one.

Out of scope: tables (filter/nat/mangle/raw), chain priorities, user-defined
chains with jump/goto, NAT. The new concept is only the hook point.

Run: python3 step5_chains.py
"""

from __future__ import annotations

from dataclasses import dataclass, field
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


class Hook(Enum):
    PREROUTING = auto()
    INPUT = auto()
    FORWARD = auto()
    OUTPUT = auto()
    POSTROUTING = auto()


@dataclass(frozen=True)
class Packet:
    src_ip: str
    dst_ip: str
    proto: str
    src_port: Optional[int] = None
    dst_port: Optional[int] = None
    tcp_flags: FrozenSet[str] = frozenset()
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
        ep1: Tuple[str, Optional[int]] = (pkt.src_ip, pkt.src_port)
        ep2: Tuple[str, Optional[int]] = (pkt.dst_ip, pkt.dst_port)
        if ep1 <= ep2:
            return ConnKey(pkt.proto, ep1[0], ep1[1], ep2[0], ep2[1])
        return ConnKey(pkt.proto, ep2[0], ep2[1], ep1[0], ep1[1])


def is_connection_opener(pkt: Packet) -> bool:
    return pkt.proto == "tcp" and "SYN" in pkt.tcp_flags and "ACK" not in pkt.tcp_flags


class Conntrack:
    """Global table, shared by every hook."""

    def __init__(self) -> None:
        self.table: Dict[ConnKey, CtState] = {}

    def state_of(self, pkt: Packet) -> CtState:
        if ConnKey.from_packet(pkt) in self.table:
            return CtState.ESTABLISHED
        if pkt.embedded is not None and ConnKey.from_packet(pkt.embedded) in self.table:
            return CtState.RELATED
        if pkt.proto == "tcp":
            return CtState.NEW if is_connection_opener(pkt) else CtState.INVALID
        return CtState.NEW

    def commit(self, pkt: Packet) -> None:
        if pkt.proto in ("tcp", "udp"):
            self.table[ConnKey.from_packet(pkt)] = CtState.ESTABLISHED


@dataclass(frozen=True)
class Rule:
    name: str
    match: Callable[[Packet, CtState], bool]
    verdict: Verdict


@dataclass
class Chain:
    """All rules that run at one hook point."""

    hook: Hook
    rules: List[Rule]
    default: Verdict = Verdict.DROP

    def decide(self, pkt: Packet, ct_state: CtState) -> Verdict:
        for rule in self.rules:
            if rule.match(pkt, ct_state):
                return rule.verdict
        return self.default


class Netfilter:
    """The set of chains, plus the shared conntrack table."""

    def __init__(self, chains: List[Chain]) -> None:
        self.chains: Dict[Hook, Chain] = {c.hook: c for c in chains}
        self.conntrack = Conntrack()

    def _traverse(self, pkt: Packet, path: List[Hook]) -> Tuple[Verdict, List[Tuple[Hook, CtState, Verdict]]]:
        # Compute the state once; every hook sees the same value.
        ct_state = self.conntrack.state_of(pkt)
        trace: List[Tuple[Hook, CtState, Verdict]] = []

        for hook in path:
            chain = self.chains.get(hook)
            if chain is None:
                # No rules at this hook: the packet simply passes.
                trace.append((hook, ct_state, Verdict.ACCEPT))
                continue
            verdict = chain.decide(pkt, ct_state)
            trace.append((hook, ct_state, verdict))
            if verdict is Verdict.DROP:
                return Verdict.DROP, trace  # traversal stops at the first DROP

        self.conntrack.commit(pkt)
        return Verdict.ACCEPT, trace

    def outbound(self, pkt: Packet):
        return self._traverse(pkt, [Hook.OUTPUT, Hook.POSTROUTING])

    def inbound_local(self, pkt: Packet):
        return self._traverse(pkt, [Hook.PREROUTING, Hook.INPUT])

    def forward(self, pkt: Packet):
        return self._traverse(pkt, [Hook.PREROUTING, Hook.FORWARD, Hook.POSTROUTING])


def describe(pkt: Packet) -> str:
    port = f":{pkt.dst_port}" if pkt.dst_port is not None else ""
    flags = f" [{','.join(sorted(pkt.tcp_flags))}]" if pkt.tcp_flags else ""
    text = f"{pkt.src_ip} -> {pkt.dst_ip} [{pkt.proto}{port}]{flags}"
    if pkt.embedded is not None:
        text += f" (quotes {pkt.embedded.src_ip}->{pkt.embedded.dst_ip})"
    return text


def main() -> None:
    nf = Netfilter(
        chains=[
            # PREROUTING: runs before routing. Nothing special here.
            Chain(Hook.PREROUTING, rules=[], default=Verdict.ACCEPT),
            # INPUT: packets delivered to the local host.
            Chain(
                Hook.INPUT,
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
            ),
            # FORWARD: this host is not a router, so forwarding is denied.
            Chain(Hook.FORWARD, rules=[], default=Verdict.DROP),
            # OUTPUT: packets generated locally.
            Chain(
                Hook.OUTPUT,
                rules=[
                    Rule("allow_established", lambda p, s: s == CtState.ESTABLISHED, Verdict.ACCEPT),
                    Rule("allow_new_out", lambda p, s: s == CtState.NEW, Verdict.ACCEPT),
                ],
                default=Verdict.DROP,
            ),
            # POSTROUTING: runs after routing, just before the wire.
            Chain(Hook.POSTROUTING, rules=[], default=Verdict.ACCEPT),
        ]
    )

    local = "192.168.1.10"
    peer = "93.184.216.34"

    def run(title: str, kind: str, pkt: Packet) -> None:
        runner = {
            "out": nf.outbound,
            "in": nf.inbound_local,
            "fwd": nf.forward,
        }[kind]
        verdict, trace = runner(pkt)
        print(f"{title}: {describe(pkt)}")
        for hook, state, v in trace:
            print(f"    {hook.name:<12} ct={state.name:<12} {v.name}")
        print(f"    => {verdict.name}\n")

    print("=== A. Locally generated packet: OUTPUT -> POSTROUTING ===\n")
    run("outbound HTTP", "out", Packet(local, peer, "tcp", 54321, 80, frozenset({"SYN"})))

    print("=== B. Reply delivered to the local host: PREROUTING -> INPUT ===\n")
    run("peer reply", "in", Packet(peer, local, "tcp", 80, 54321, frozenset({"SYN", "ACK"})))

    print("=== C. New inbound SSH: PREROUTING -> INPUT ===\n")
    run("new SSH", "in", Packet(peer, local, "tcp", 40000, 22, frozenset({"SYN"})))

    print("=== D. Stray ACK dropped at INPUT ===\n")
    run("stray ACK", "in", Packet(peer, local, "tcp", 40001, 22, frozenset({"ACK"})))

    print("=== E. Transit packet: PREROUTING -> FORWARD -> POSTROUTING ===\n")
    run("forwarded", "fwd", Packet("198.51.100.7", "203.0.113.9", "tcp", 50000, 80, frozenset({"SYN"})))

    print(f"conntrack holds {len(nf.conntrack.table)} connections.")
    print("Result: the same rule machinery now runs at several hook points, and")
    print("the packet path depends on routing (INPUT vs FORWARD). A DROP at any")
    print("hook stops the traversal; conntrack state is computed once per packet.")
    print("\nNext pain point: at each hook we currently have one flat list. Real")
    print("Linux groups chains into TABLES with different purposes (filter, nat,")
    print("mangle) and priorities -- the 'table' concept.")


if __name__ == "__main__":
    main()
