#!/usr/bin/env python3
"""Step 8 (cumulative): add NAT to the step-7 engine.

Companion to the "Linux firewall" lesson, step eight.

This file is step 7 PLUS address translation. Everything from step 7 is still
here (Hook, Table, Chain, user chains, JUMP, RETURN). New pieces are marked
with "# --- NEW" and changed pieces with "# --- CHANGED" so you can see exactly
what it takes to add NAT.

What it takes to add NAT, in one sentence:
    The traversal must carry a MUTABLE flow (current packet + original packet)
    instead of a bare Packet, and the nat table needs a rule action that
    REWRITES the packet (non-terminal) plus a conntrack binding that reverses
    the rewrite for replies.

New concepts:
    - Binding            : one original <-> translated endpoint mapping.
    - Conntrack.nat_bindings + binding_for_forward / binding_for_reply.
    - NatAction          : a rule action that rewrites and continues.
    - Flow               : {orig, pkt, ct_state} threaded through the engine.
Changed concepts:
    - Action union now also allows NatAction.
    - _exec / _traverse take a Flow; _traverse un-NATs replies first and
      commits using the ORIGINAL (pre-NAT) tuple.

Run: python3 step8_nat.py
"""

from __future__ import annotations

from dataclasses import dataclass, replace
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


# --- NEW: a NAT mapping between original and translated endpoints. ---
@dataclass(frozen=True)
class Binding:
    kind: str  # "DNAT" changes the destination, "SNAT" the source
    orig_src: Tuple[str, Optional[int]]
    orig_dst: Tuple[str, Optional[int]]
    xlate_src: Tuple[str, Optional[int]]
    xlate_dst: Tuple[str, Optional[int]]


class Conntrack:
    def __init__(self) -> None:
        self.table: Dict[ConnKey, CtState] = {}
        self.nat_bindings: List[Binding] = []  # --- NEW ---

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

    # --- NEW: lookup helpers used by the traversal. ---
    def binding_for_forward(self, pkt: Packet) -> Optional[Binding]:
        here = ((pkt.src_ip, pkt.src_port), (pkt.dst_ip, pkt.dst_port))
        for b in self.nat_bindings:
            if here == (b.orig_src, b.orig_dst):
                return b
        return None

    def binding_for_reply(self, pkt: Packet) -> Optional[Binding]:
        here = ((pkt.src_ip, pkt.src_port), (pkt.dst_ip, pkt.dst_port))
        for b in self.nat_bindings:
            if here == (b.xlate_dst, b.xlate_src):
                return b
        return None


@dataclass(frozen=True)
class Jump:
    """Non-terminal target: run another chain, then come back here."""

    chain: str


# --- NEW: a rule action that rewrites the packet and keeps going. ---
@dataclass(frozen=True)
class NatAction:
    kind: str                       # "DNAT" or "SNAT"
    new_ip: str
    new_port: Optional[int] = None  # None => keep the original port


# --- CHANGED: the action union now also allows a NatAction. ---
Action = Union[Verdict, Jump, NatAction]


@dataclass(frozen=True)
class Rule:
    name: str
    match: Callable[[Packet, CtState], bool]
    action: Action


@dataclass
class Chain:
    """Built-in chains have a hook and a policy; user chains have neither,
    so reaching their end is a RETURN."""

    table: Table
    name: str
    rules: List[Rule]
    hook: Optional[Hook] = None
    policy: Optional[Verdict] = None

    @property
    def is_builtin(self) -> bool:
        return self.hook is not None


# --- NEW: the mutable context threaded through the traversal. ---
@dataclass
class Flow:
    orig: Packet                 # pre-NAT packet, used as the conntrack key
    pkt: Packet                  # current packet (NAT may replace it)
    ct_state: Optional[CtState] = None


class Netfilter:
    def __init__(self, chains: List[Chain]) -> None:
        self.chains: Dict[Tuple[Table, str], Chain] = {(c.table, c.name): c for c in chains}
        self.conntrack = Conntrack()

    def _tables_at(self, hook: Hook) -> List[Table]:
        tables = {c.table for c in self.chains.values() if c.hook is hook}
        return sorted(tables, key=lambda t: t.priority)

    # --- NEW: NAT translation helpers. ---
    @staticmethod
    def _replace(pkt: Packet, src, dst) -> Packet:
        return replace(pkt, src_ip=src[0], src_port=src[1], dst_ip=dst[0], dst_port=dst[1])

    def _make_binding(self, pkt: Packet, action: NatAction) -> Binding:
        orig_src = (pkt.src_ip, pkt.src_port)
        orig_dst = (pkt.dst_ip, pkt.dst_port)
        if action.kind == "DNAT":
            xlate_dst = (action.new_ip, action.new_port if action.new_port is not None else pkt.dst_port)
            return Binding("DNAT", orig_src, orig_dst, orig_src, xlate_dst)
        xlate_src = (action.new_ip, action.new_port if action.new_port is not None else pkt.src_port)
        return Binding("SNAT", orig_src, orig_dst, xlate_src, orig_dst)

    def _apply_nat(self, flow: Flow, action: NatAction) -> Binding:
        b = self._make_binding(flow.pkt, action)
        self.conntrack.nat_bindings.append(b)
        flow.pkt = self._replace(flow.pkt, b.xlate_src, b.xlate_dst)
        return b

    def _reverse_nat(self, pkt: Packet) -> Tuple[Packet, bool]:
        b = self.conntrack.binding_for_reply(pkt)
        if b is not None:
            return self._replace(pkt, b.orig_dst, b.orig_src), True
        return pkt, False

    def _translate_existing(self, flow: Flow, kind: str) -> bool:
        b = self.conntrack.binding_for_forward(flow.pkt)
        if b is not None and b.kind == kind:
            flow.pkt = self._replace(flow.pkt, b.xlate_src, b.xlate_dst)
            return True
        return False

    # --- CHANGED: takes a Flow instead of a Packet; handles NatAction. ---
    def _exec(
        self,
        table: Table,
        name: str,
        flow: Flow,
        trace: List[Tuple[int, str, CtState, str]],
        depth: int = 0,
    ) -> Optional[Verdict]:
        label = f"{table.name}/{name}"
        chain = self.chains.get((table, name))
        if chain is None:
            trace.append((depth, label, flow.ct_state, "undefined -> pass"))
            return None

        kind = "built-in" if chain.is_builtin else "user"
        policy = chain.policy.name if chain.policy else "none"
        trace.append((depth, label, flow.ct_state, f"enter ({kind}, policy={policy})"))

        for rule in chain.rules:
            if not rule.match(flow.pkt, flow.ct_state):
                continue

            if isinstance(rule.action, Jump):
                trace.append((depth, label, flow.ct_state, f"rule '{rule.name}' -> JUMP {rule.action.chain}"))
                result = self._exec(table, rule.action.chain, flow, trace, depth + 1)
                if result is not None:
                    trace.append((depth, label, flow.ct_state, f"<- {result.name} (terminal)"))
                    return result
                trace.append((depth, label, flow.ct_state, "<- RETURN"))
                continue

            # --- NEW: a NAT action rewrites the packet, then the chain ends. ---
            if isinstance(rule.action, NatAction):
                self._apply_nat(flow, rule.action)
                trace.append((depth, label, flow.ct_state,
                              f"rule '{rule.name}' -> {rule.action.kind} {rule.action.new_ip}"
                              f" -> {describe(flow.pkt)} (non-terminal)"))
                break

            if rule.action is Verdict.RETURN:
                trace.append((depth, label, flow.ct_state, f"rule '{rule.name}' -> RETURN"))
                return None

            trace.append((depth, label, flow.ct_state, f"rule '{rule.name}' -> {rule.action.name}"))
            return rule.action

        if chain.policy is not None:
            trace.append((depth, label, flow.ct_state, f"end -> policy {chain.policy.name}"))
            return chain.policy
        trace.append((depth, label, flow.ct_state, "end -> RETURN (no policy)"))
        return None

    # --- CHANGED: builds a Flow, un-NATs replies, commits the original tuple. ---
    def _traverse(self, pkt: Packet, path: List[Hook]):
        pkt, was_reply = self._reverse_nat(pkt)
        flow = Flow(orig=pkt, pkt=pkt)
        flow.ct_state = self.conntrack.state_of(pkt)
        trace: List[Tuple[int, str, CtState, str]] = []
        trace.append((0, "conntrack", flow.ct_state,
                      f"un-NAT (reply) -> {describe(flow.pkt)}" if was_reply else "classify"))

        for hook in path:
            for table in self._tables_at(hook):
                if table is Table.NAT:
                    # nat chains are only consulted for NEW; established packets
                    # get the stored mapping applied by conntrack.
                    if flow.ct_state is CtState.NEW:
                        result = self._exec(table, hook.name, flow, trace)
                        if result is Verdict.DROP:
                            return Verdict.DROP, trace
                    else:
                        kind = "DNAT" if hook is Hook.PREROUTING else "SNAT"
                        changed = self._translate_existing(flow, kind)
                        trace.append((0, f"{table.name}/{hook.name}", flow.ct_state,
                                      f"NAT existing -> {describe(flow.pkt)}" if changed else "NAT skip"))
                    continue

                result = self._exec(table, hook.name, flow, trace)
                if result is Verdict.DROP:
                    return Verdict.DROP, trace

        self.conntrack.commit(flow.orig)  # key by the ORIGINAL (pre-NAT) tuple
        return Verdict.ACCEPT, trace

    def inbound_local(self, pkt: Packet):
        return self._traverse(pkt, [Hook.PREROUTING, Hook.INPUT])

    def outbound(self, pkt: Packet):
        return self._traverse(pkt, [Hook.OUTPUT, Hook.POSTROUTING])

    def forward(self, pkt: Packet):
        return self._traverse(pkt, [Hook.PREROUTING, Hook.FORWARD, Hook.POSTROUTING])


def describe(pkt: Packet) -> str:
    return f"{pkt.src_ip}:{pkt.src_port} -> {pkt.dst_ip}:{pkt.dst_port} [{pkt.proto}]"


def main() -> None:
    nf = Netfilter(
        chains=[
            # --- nat table: DNAT at PREROUTING (new in step 8). ---
            Chain(
                table=Table.NAT,
                name=Hook.PREROUTING.name,
                hook=Hook.PREROUTING,
                policy=Verdict.ACCEPT,
                rules=[
                    Rule(
                        "web-forward",
                        lambda p, s: p.proto == "tcp" and p.dst_ip == "203.0.113.1" and p.dst_port == 80,
                        NatAction("DNAT", new_ip="192.168.1.80", new_port=8080),
                    ),
                ],
            ),
            # --- nat table: SNAT at POSTROUTING (new in step 8). ---
            Chain(
                table=Table.NAT,
                name=Hook.POSTROUTING.name,
                hook=Hook.POSTROUTING,
                policy=Verdict.ACCEPT,
                rules=[
                    Rule(
                        "masquerade",
                        lambda p, s: p.proto == "tcp"
                        and p.src_ip.startswith("192.168.1.")
                        and p.dst_ip == "93.184.216.34",
                        NatAction("SNAT", new_ip="203.0.113.1"),
                    ),
                ],
            ),
            # --- filter INPUT (from step 7, with user chains). ---
            Chain(
                table=Table.FILTER,
                name=Hook.INPUT.name,
                hook=Hook.INPUT,
                policy=Verdict.DROP,
                rules=[
                    Rule("drop_invalid", lambda p, s: s == CtState.INVALID, Verdict.DROP),
                    Rule("allow_established", lambda p, s: s == CtState.ESTABLISHED, Verdict.ACCEPT),
                    Rule("allow_related", lambda p, s: s == CtState.RELATED, Verdict.ACCEPT),
                    Rule("check_blocklist", lambda p, s: p.proto == "tcp", Jump("blocklist")),
                    Rule("check_services", lambda p, s: s == CtState.NEW, Jump("allow_services")),
                ],
            ),
            Chain(
                table=Table.FILTER,
                name="blocklist",
                rules=[
                    Rule("drop_bogus", lambda p, s: p.src_ip == "10.0.0.66", Verdict.DROP),
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
            # --- filter FORWARD: must allow the DNATed and SNATed flows. ---
            Chain(
                table=Table.FILTER,
                name=Hook.FORWARD.name,
                hook=Hook.FORWARD,
                policy=Verdict.DROP,
                rules=[
                    Rule("allow_established", lambda p, s: s == CtState.ESTABLISHED, Verdict.ACCEPT),
                    Rule("allow_related", lambda p, s: s == CtState.RELATED, Verdict.ACCEPT),
                    Rule("allow_new_to_web", lambda p, s: s == CtState.NEW and p.dst_port == 8080, Verdict.ACCEPT),
                    Rule("allow_new_from_lan", lambda p, s: s == CtState.NEW and p.src_ip.startswith("192.168.1."), Verdict.ACCEPT),
                ],
            ),
            # --- filter OUTPUT. ---
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

    print("=== From step 7: user chains still work (filter only) ===\n")
    run("new SSH", "in", Packet(peer, local, "tcp", 40000, 22, frozenset({"SYN"})))
    run("new HTTP", "in", Packet(peer, local, "tcp", 40001, 80, frozenset({"SYN"})))

    print("=== New in step 8: DNAT at PREROUTING, reversed for the reply ===\n")
    run("client -> public", "fwd", Packet("198.51.100.7", "203.0.113.1", "tcp", 40000, 80, frozenset({"SYN"})))
    run("server -> client", "fwd", Packet("192.168.1.80", "198.51.100.7", "tcp", 8080, 40000, frozenset({"SYN", "ACK"})))

    print("=== New in step 8: SNAT at POSTROUTING, reversed for the reply ===\n")
    run("lan -> internet", "fwd", Packet(local, peer, "tcp", 54321, 80, frozenset({"SYN"})))
    run("internet -> public", "fwd", Packet(peer, "203.0.113.1", "tcp", 80, 54321, frozenset({"SYN", "ACK"})))

    print(f"conntrack holds {len(nf.conntrack.table)} connections, {len(nf.conntrack.nat_bindings)} NAT bindings.")
    print("Result: the step-7 engine is unchanged in shape. To add NAT we only")
    print("threaded a mutable Flow, added a non-terminal NatAction, and stored a")
    print("Binding that conntrack reverses for replies.")
    print("\nNext pain point: this model still decides everything in user space and")
    print("walks one rule at a time. Real Linux matches packets in the kernel at")
    print("line rate -- that is the userspace/kernel split and the real tooling.")


if __name__ == "__main__":
    main()
