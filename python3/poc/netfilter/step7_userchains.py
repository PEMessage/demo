#!/usr/bin/env python3
"""Step 7: user-defined chains, JUMP and RETURN.

Companion to the "Linux firewall" lesson, step seven.

Pain point from the previous discussion:
    A chain can have NO policy at all. A built-in chain must resolve to
    ACCEPT or DROP, but a user-defined chain has no policy: reaching its end
    means "no opinion", i.e. RETURN to the caller.

The new concept is the user-defined chain with non-terminal verdicts:
    - built-in chain: has a policy (ACCEPT / DROP), it is a hook entry point.
    - user-defined chain: no policy, reached via a Jump from another chain.
    - Jump   (-j name): push a return address, run the target chain.
    - RETURN: pop, resume the caller's next rule.
    - ACCEPT / DROP inside a user chain stay TERMINAL for the whole hook:
      DROP kills the packet, ACCEPT accepts it at this hook.

A jump does not need its own verdict: if the user chain ends (or returns),
the caller simply keeps going with its next rule.

Out of scope: GOTO (-g, jump without a return address), per-table chain
namespaces (here user chains live in the filter table), MATCH extensions.

Run: python3 step7_userchains.py
"""

from __future__ import annotations

from dataclasses import dataclass
from enum import Enum, auto
from typing import Callable, Dict, FrozenSet, List, Optional, Tuple, Union


class Verdict(Enum):
    # ACCEPT / DROP are terminal. RETURN is non-terminal: back to the caller.
    ACCEPT = auto()
    DROP = auto()
    RETURN = auto()


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
    """Netfilter tables: priority, typical hooks, and responsibility.

    raw       PREROUTING, OUTPUT                  before conntrack; NOTRACK
    mangle    all five hooks                       header edits: TTL, MARK
    nat       PREROUTING/OUTPUT/POSTROUTING        address rewrite, first packet
    filter    INPUT, FORWARD, OUTPUT               accept/drop decisions
    """

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
class Jump:
    """Non-terminal target: run another chain, then come back here."""

    chain: str


# A rule action is either a Verdict (ACCEPT/DROP/RETURN) or a Jump.
Action = Union[Verdict, Jump]


@dataclass(frozen=True)
class Rule:
    name: str
    match: Callable[[Packet, CtState], bool]
    action: Action


@dataclass
class Chain:
    """One chain. Built-in chains have a hook and a policy; user chains have
    neither (hook=None, policy=None), so reaching their end is a RETURN."""

    table: Table
    name: str
    rules: List[Rule]
    hook: Optional[Hook] = None
    policy: Optional[Verdict] = None

    @property
    def is_builtin(self) -> bool:
        return self.hook is not None


class Netfilter:
    def __init__(self, chains: List[Chain]) -> None:
        self.chains: Dict[Tuple[Table, str], Chain] = {(c.table, c.name): c for c in chains}
        self.conntrack = Conntrack()

    def _tables_at(self, hook: Hook) -> List[Table]:
        tables = {c.table for c in self.chains.values() if c.hook is hook}
        return sorted(tables, key=lambda t: t.priority)

    def _exec(
        self,
        table: Table,
        name: str,
        pkt: Packet,
        ct_state: CtState,
        trace: List[Tuple[int, str, CtState, str]],
        depth: int = 0,
    ) -> Optional[Verdict]:
        """Run one chain. Returns a terminal verdict (ACCEPT/DROP), or None
        meaning 'returned / fell off the end without an opinion'."""
        label = f"{table.name}/{name}"
        chain = self.chains.get((table, name))
        if chain is None:
            trace.append((depth, label, ct_state, "undefined -> pass"))
            return None

        kind = "built-in" if chain.is_builtin else "user"
        policy = chain.policy.name if chain.policy else "none"
        trace.append((depth, label, ct_state, f"enter ({kind}, policy={policy})"))

        for rule in chain.rules:
            if not rule.match(pkt, ct_state):
                continue

            if isinstance(rule.action, Jump):
                trace.append((depth, label, ct_state, f"rule '{rule.name}' -> JUMP {rule.action.chain}"))
                result = self._exec(table, rule.action.chain, pkt, ct_state, trace, depth + 1)
                if result is not None:
                    trace.append((depth, label, ct_state, f"<- {result.name} (terminal)"))
                    return result
                trace.append((depth, label, ct_state, "<- RETURN"))
                continue

            if rule.action is Verdict.RETURN:
                trace.append((depth, label, ct_state, f"rule '{rule.name}' -> RETURN"))
                return None

            trace.append((depth, label, ct_state, f"rule '{rule.name}' -> {rule.action.name}"))
            return rule.action

        # No rule matched: built-in chains apply their policy, user chains return.
        if chain.policy is not None:
            trace.append((depth, label, ct_state, f"end -> policy {chain.policy.name}"))
            return chain.policy
        trace.append((depth, label, ct_state, "end -> RETURN (no policy)"))
        return None

    def _traverse(self, pkt: Packet, path: List[Hook]):
        ct_state = self.conntrack.state_of(pkt)
        trace: List[Tuple[int, str, CtState, str]] = []

        for hook in path:
            for table in self._tables_at(hook):
                if table is Table.NAT and ct_state is not CtState.NEW:
                    trace.append((0, f"{table.name}/{hook.name}", ct_state, "SKIP (not NEW)"))
                    continue
                result = self._exec(table, hook.name, pkt, ct_state, trace)
                if result is Verdict.DROP:
                    return Verdict.DROP, trace

        self.conntrack.commit(pkt)
        return Verdict.ACCEPT, trace

    def inbound_local(self, pkt: Packet):
        return self._traverse(pkt, [Hook.PREROUTING, Hook.INPUT])

    def outbound(self, pkt: Packet):
        return self._traverse(pkt, [Hook.OUTPUT, Hook.POSTROUTING])

    def forward(self, pkt: Packet):
        return self._traverse(pkt, [Hook.PREROUTING, Hook.FORWARD, Hook.POSTROUTING])


def describe(pkt: Packet) -> str:
    port = f":{pkt.dst_port}" if pkt.dst_port is not None else ""
    flags = f" [{','.join(sorted(pkt.tcp_flags))}]" if pkt.tcp_flags else ""
    return f"{pkt.src_ip} -> {pkt.dst_ip} [{pkt.proto}{port}]{flags}"


def main() -> None:
    nf = Netfilter(
        chains=[
            # --- Built-in chain: INPUT, policy DROP (whitelist). ---
            Chain(
                table=Table.FILTER,
                name=Hook.INPUT.name,
                hook=Hook.INPUT,
                policy=Verdict.DROP,
                rules=[
                    Rule("drop_invalid", lambda p, s: s == CtState.INVALID, Verdict.DROP),
                    Rule("allow_established", lambda p, s: s == CtState.ESTABLISHED, Verdict.ACCEPT),
                    Rule("allow_related", lambda p, s: s == CtState.RELATED, Verdict.ACCEPT),
                    # Jump into a user chain; it may DROP, otherwise RETURN.
                    Rule("check_blocklist", lambda p, s: p.proto == "tcp", Jump("blocklist")),
                    # Jump into a user chain that only ACCEPTs known services.
                    Rule("check_services", lambda p, s: s == CtState.NEW, Jump("allow_services")),
                ],
            ),
            # --- User-defined chain: no hook, no policy. ---
            Chain(
                table=Table.FILTER,
                name="blocklist",
                rules=[
                    Rule("drop_bogus", lambda p, s: p.src_ip == "10.0.0.66", Verdict.DROP),
                    # Explicit RETURN: give up on this chain early.
                    Rule("skip_local", lambda p, s: p.src_ip.startswith("127."), Verdict.RETURN),
                ],
            ),
            Chain(
                table=Table.FILTER,
                name="allow_services",
                rules=[
                    Rule("allow_ssh", lambda p, s: p.dst_port == 22, Verdict.ACCEPT),
                    Rule("allow_https", lambda p, s: p.dst_port == 443, Verdict.ACCEPT),
                ],
            ),
            # --- Other built-in chains (filter). ---
            Chain(Table.FILTER, Hook.FORWARD.name, hook=Hook.FORWARD, policy=Verdict.DROP, rules=[]),
            Chain(
                table=Table.FILTER,
                name=Hook.OUTPUT.name,
                hook=Hook.OUTPUT,
                policy=Verdict.DROP,
                rules=[
                    Rule("allow_established", lambda p, s: s == CtState.ESTABLISHED, Verdict.ACCEPT),
                    Rule("allow_new_out", lambda p, s: s == CtState.NEW, Verdict.ACCEPT),
                ],
            ),
        ]
    )

    local = "192.168.1.10"
    peer = "93.184.216.34"

    def run(title: str, kind: str, pkt: Packet) -> None:
        runner = {"in": nf.inbound_local, "out": nf.outbound, "fwd": nf.forward}[kind]
        verdict, trace = runner(pkt)
        print(f"{title}: {describe(pkt)}")
        for depth, label, state, text in trace:
            print(f"    {'  ' * depth}{label:<22} ct={state.name:<12} {text}")
        print(f"    => {verdict.name}\n")

    print("user chains live in the filter table and have no policy.\n")

    print("=== A. New SSH: jump in, user chain ACCEPTs (terminal) ===\n")
    run("new SSH", "in", Packet(peer, local, "tcp", 40000, 22, frozenset({"SYN"})))

    print("=== B. New HTTP: both user chains RETURN, INPUT policy DROPs ===\n")
    run("new HTTP", "in", Packet(peer, local, "tcp", 40001, 80, frozenset({"SYN"})))

    print("=== C. Blocked source: DROP inside a user chain is terminal ===\n")
    run("blocked src", "in", Packet("10.0.0.66", local, "tcp", 40002, 22, frozenset({"SYN"})))

    print("=== D. Explicit RETURN skips the rest of the user chain ===\n")
    run("from loopback", "in", Packet("127.0.0.1", local, "tcp", 40003, 443, frozenset({"SYN"})))

    print(f"conntrack holds {len(nf.conntrack.table)} connections.")
    print("Result: a user chain needs no policy. If it ends (or hits RETURN), the")
    print("caller resumes; only the built-in chain's policy makes the final call.")
    print("ACCEPT/DROP inside a user chain are still terminal for the whole hook.")
    print("\nNext pain point: nothing here rewrites addresses. Port forwarding and")
    print("masquerading need address translation -- the nat table's real job.")


if __name__ == "__main__":
    main()
