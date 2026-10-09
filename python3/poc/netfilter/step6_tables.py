#!/usr/bin/env python3
"""Step 6: introduce tables.

Companion to the "Linux firewall" lesson, step six.

Pain point from step five:
    Each hook had exactly one flat chain. Real Linux puts SEVERAL tables at the
    same hook, ordered by priority, and the packet walks through all of them.

The new concept is the table:
    A table is a named group of rules with a purpose and a priority. At one
    hook the tables run in priority order, e.g.
        raw(-300) -> mangle(-150) -> nat(-100) -> filter(0)
    ACCEPT in one table does NOT skip the next table; only DROP terminates.

Engineering detail that surfaces once conntrack and tables coexist:
    The nat table is consulted only for the FIRST packet of a connection
    (ct state NEW). Later packets of the same connection skip nat entirely.

Out of scope: the actual effects of mangle (MARK, TTL) and nat (SNAT/DNAT) are
their own concepts. Here the tables only decide ACCEPT/DROP, to show grouping,
ordering and traversal.

Run: python3 step6_tables.py
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


class Hook(Enum):
    PREROUTING = auto()
    INPUT = auto()
    FORWARD = auto()
    OUTPUT = auto()
    POSTROUTING = auto()


class Table(Enum):
    # Value is the priority: lower runs first, matching real netfilter.
    RAW = -300
    MANGLE = -150
    NAT = -100
    FILTER = 0

    @property
    def priority(self) -> int:
        return self.value


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
    """All rules of one table at one hook."""

    table: Table
    hook: Hook
    rules: List[Rule]
    default: Verdict = Verdict.ACCEPT

    def decide(self, pkt: Packet, ct_state: CtState) -> Verdict:
        for rule in self.rules:
            if rule.match(pkt, ct_state):
                return rule.verdict
        return self.default


class Netfilter:
    def __init__(self, chains: List[Chain]) -> None:
        self.chains: Dict[Tuple[Table, Hook], Chain] = {(c.table, c.hook): c for c in chains}
        self.conntrack = Conntrack()

    def _tables_at(self, hook: Hook) -> List[Table]:
        tables = {t for (t, h) in self.chains if h is hook}
        return sorted(tables, key=lambda t: t.priority)

    def _traverse(self, pkt: Packet, path: List[Hook]):
        ct_state = self.conntrack.state_of(pkt)
        trace: List[Tuple[Hook, Optional[Table], CtState, str]] = []

        for hook in path:
            for table in self._tables_at(hook):
                # nat only sees the first packet of a connection.
                if table is Table.NAT and ct_state is not CtState.NEW:
                    trace.append((hook, table, ct_state, "SKIP"))
                    continue
                chain = self.chains[(table, hook)]
                verdict = chain.decide(pkt, ct_state)
                trace.append((hook, table, ct_state, verdict.name))
                if verdict is Verdict.DROP:
                    return Verdict.DROP, trace  # DROP terminates the whole traversal

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
    return f"{pkt.src_ip} -> {pkt.dst_ip} [{pkt.proto}{port}]{flags}"


def main() -> None:
    def empty(table: Table, hook: Hook) -> Chain:
        return Chain(table, hook, rules=[], default=Verdict.ACCEPT)

    nf = Netfilter(
        chains=[
            # PREROUTING: raw -> mangle -> nat (before routing).
            empty(Table.RAW, Hook.PREROUTING),
            Chain(
                Table.MANGLE,
                Hook.PREROUTING,
                rules=[
                    # A rule living in mangle: dropping here stops nat and filter.
                    Rule("drop_bogus_src", lambda p, s: p.src_ip == "10.0.0.66", Verdict.DROP),
                ],
                default=Verdict.ACCEPT,
            ),
            empty(Table.NAT, Hook.PREROUTING),
            # INPUT: filter only.
            Chain(
                Table.FILTER,
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
            # FORWARD: filter only; not a router, so deny.
            Chain(Table.FILTER, Hook.FORWARD, rules=[], default=Verdict.DROP),
            # OUTPUT: raw -> mangle -> nat -> filter.
            empty(Table.RAW, Hook.OUTPUT),
            empty(Table.MANGLE, Hook.OUTPUT),
            empty(Table.NAT, Hook.OUTPUT),
            Chain(
                Table.FILTER,
                Hook.OUTPUT,
                rules=[
                    Rule("allow_established", lambda p, s: s == CtState.ESTABLISHED, Verdict.ACCEPT),
                    Rule("allow_new_out", lambda p, s: s == CtState.NEW, Verdict.ACCEPT),
                ],
                default=Verdict.DROP,
            ),
            # POSTROUTING: mangle -> nat.
            empty(Table.MANGLE, Hook.POSTROUTING),
            empty(Table.NAT, Hook.POSTROUTING),
        ]
    )

    local = "192.168.1.10"
    peer = "93.184.216.34"

    def run(title: str, kind: str, pkt: Packet) -> None:
        runner = {"out": nf.outbound, "in": nf.inbound_local, "fwd": nf.forward}[kind]
        verdict, trace = runner(pkt)
        print(f"{title}: {describe(pkt)}")
        for hook, table, state, v in trace:
            name = f"{hook.name}/{table.name}" if table else hook.name
            print(f"    {name:<24} ct={state.name:<12} {v}")
        print(f"    => {verdict.name}\n")

    print("table priority: raw(-300) -> mangle(-150) -> nat(-100) -> filter(0)\n")

    print("=== A. Outbound HTTP (NEW): all tables at OUTPUT, then POSTROUTING ===\n")
    run("outbound HTTP", "out", Packet(local, peer, "tcp", 54321, 80, frozenset({"SYN"})))

    print("=== B. Reply to local (ESTABLISHED): nat is skipped at PREROUTING ===\n")
    run("peer reply", "in", Packet(peer, local, "tcp", 80, 54321, frozenset({"SYN", "ACK"})))

    print("=== C. New inbound SSH (NEW): nat runs, then filter decides ===\n")
    run("new SSH", "in", Packet(peer, local, "tcp", 40000, 22, frozenset({"SYN"})))

    print("=== D. DROP in mangle stops nat and filter ===\n")
    run("bogus source", "in", Packet("10.0.0.66", local, "tcp", 40001, 22, frozenset({"SYN"})))

    print("=== E. Stray ACK: filter drops INVALID ===\n")
    run("stray ACK", "in", Packet(peer, local, "tcp", 40002, 22, frozenset({"ACK"})))

    print(f"conntrack holds {len(nf.conntrack.table)} connections.")
    print("Result: several tables can sit at one hook, ordered by priority. ACCEPT")
    print("in one table falls through to the next; DROP stops everything. The nat")
    print("table only sees NEW packets, so established traffic skips it.")
    print("\nNext pain point: tables can only accept/drop so far. Rewriting source")
    print("or destination addresses (NAT) is a new concept -- the nat table's job.")


if __name__ == "__main__":
    main()
