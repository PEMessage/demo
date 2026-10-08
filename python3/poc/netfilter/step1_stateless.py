#!/usr/bin/env python3
"""Step 1: stateless packet filter.

Companion to the "Linux firewall" lesson, step one.

Core concept (only one introduced):
    A rule list is scanned in order; the first matching rule wins.
    If nothing matches, the default policy applies.

Object boundaries:
    Packet   -- read-only packet metadata, lifetime = one decision
    Rule     -- match function + verdict; the match function is pure
    Firewall -- rule list + default policy
    Verdict  -- step one only has ACCEPT / DROP

Out of scope for this step: connection state, NAT, interface direction,
logging, rate limiting, performance.

Run: python3 step1_stateless.py
"""

from __future__ import annotations

from dataclasses import dataclass
from enum import Enum, auto
from typing import Callable, List, Optional


class Verdict(Enum):
    ACCEPT = auto()
    DROP = auto()


@dataclass(frozen=True)
class Packet:
    """Minimal packet view. Step-one boundary: no payload, no conntrack state."""

    src_ip: str
    dst_ip: str
    proto: str                      # "tcp" / "udp" / "icmp"
    dst_port: Optional[int] = None


@dataclass(frozen=True)
class Rule:
    name: str
    match: Callable[[Packet], bool]
    verdict: Verdict


class Firewall:
    def __init__(self, rules: List[Rule], default: Verdict = Verdict.DROP):
        self.rules = rules
        self.default = default

    def decide(self, pkt: Packet) -> Verdict:
        # Core logic: scan in order, first match wins immediately.
        for rule in self.rules:
            if rule.match(pkt):
                return rule.verdict
        # No rule matched: fall back to the default policy.
        return self.default


def describe(pkt: Packet) -> str:
    port = f":{pkt.dst_port}" if pkt.dst_port is not None else ""
    return f"{pkt.src_ip} -> {pkt.dst_ip} [{pkt.proto}{port}]"


def main() -> None:
    # Config: allow inbound SSH and ICMP, drop everything else by default.
    fw = Firewall(
        rules=[
            Rule(
                "allow_ssh",
                lambda p: p.proto == "tcp" and p.dst_port == 22,
                Verdict.ACCEPT,
            ),
            Rule(
                "allow_icmp",
                lambda p: p.proto == "icmp",
                Verdict.ACCEPT,
            ),
        ],
        default=Verdict.DROP,
    )

    cases = [
        Packet("203.0.113.5", "192.168.1.10", "tcp", 22),   # new SSH   -> ACCEPT
        Packet("203.0.113.5", "192.168.1.10", "icmp"),      # ping      -> ACCEPT
        Packet("203.0.113.5", "192.168.1.10", "tcp", 80),   # HTTP      -> DROP
        # Reply to a connection we started: destination port is a random
        # high port. A stateless firewall cannot tell it is a reply, so the
        # default policy kills it -- exactly the pain point of the next step.
        Packet("93.184.216.34", "192.168.1.10", "tcp", 54321),
    ]

    print("Rules (order matters; first match wins):")
    for i, rule in enumerate(fw.rules, 1):
        print(f"  {i}. {rule.name} -> {rule.verdict.name}")
    print(f"  default policy: {fw.default.name}\n")

    for pkt in cases:
        print(f"{describe(pkt):<48} => {fw.decide(pkt).name}")

    print("\nPain point: the last case is a reply to our own outbound connection.")
    print("With no pre-written rule it is dropped. That is what step two fixes.")


if __name__ == "__main__":
    main()
