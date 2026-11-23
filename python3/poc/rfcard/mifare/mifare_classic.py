"""MIFARE Classic —— Step 0 + 2~4 的行为模型，带两种访问位视图。

访问位同时有两种看法，二者共存、可互相转换：

  1. 机器视图（raw）: 3 字节 = 24 bit，就是卡片里真实存的那 3 个字节。
     - 12 bit 有效信息: 4 个 block，每块 3 bit (C1 C2 C3)
     - 12 bit 反相冗余: 读的时候做一致性校验
     每块的 3 bit 不是独立权限位，而是一个「条件编号」，去查固定的 8 行表。

  2. 人类视图（rules）: 每块一条 BlockRule，直接写清
     「KeyA 能做什么、KeyB 能做什么」。

转换:
    AccessBits.from_raw(b"\xff\x03\xc0")   # 机器 -> 对象
    AccessBits.from_rules(rules)           # 人类 -> 对象（不可表达时抛错）
    ab.to_raw()                            # -> 3 字节
    ab.rules / ab.to_rules()               # -> 4 条 BlockRule

真实的 8 行条件表只能表达有限组合；如果 from_rules 给的规则不在表里，
会抛 UnencodableAccess，而不是悄悄改权限。多个 raw 也可能解码成同一条
人类规则（例如尾块 101 和 111），此时 from_rules 选第一个匹配项。

注意：Step 1 的「锁位 / OTP」不在这个模型里，这是有意的。
Classic 没有 OTP；它用「密钥 + 访问位」替代了静态锁。它的 value block
也不是 OTP 计数器——它支持 Increment，不是单向的。
OTP / 锁位属于 Ultralight 那条线，应单独建模，而不是塞进 Classic。

类型映射（伪代码 -> Python）:
    Byte[16] / Byte[6]  -> NewType("Block", bytes) / NewType("Key", bytes)
    struct Addr         -> @dataclass(frozen=True) Addr
    enum Auth/Kind/Op   -> enum.Enum
    transient auth      -> field(metadata={"transient": True}) + reset_session()
    DENIED              -> AccessDenied
    INVALID             -> InvalidOperation
"""

from __future__ import annotations

from dataclasses import dataclass, field
from enum import Enum, auto
from typing import Final, NewType, Optional

BLOCK_SIZE: Final = 16
KEY_SIZE: Final = 6
SECTORS_1K: Final = 16
BLOCKS_PER_SECTOR: Final = 4
VALUE_MAX: Final = 0xFFFF_FFFF

Block = NewType("Block", bytes)
Key = NewType("Key", bytes)


def as_block(raw: bytes) -> Block:
    if len(raw) != BLOCK_SIZE:
        raise ValueError(f"block must be {BLOCK_SIZE} bytes, got {len(raw)}")
    return Block(bytes(raw))


def as_key(raw: bytes) -> Key:
    if len(raw) != KEY_SIZE:
        raise ValueError(f"key must be {KEY_SIZE} bytes, got {len(raw)}")
    return Key(bytes(raw))


# ---------------------------------------------------------------- enums

class Auth(Enum):
    NONE = auto()
    KEY_A = auto()
    KEY_B = auto()


class Kind(Enum):
    DATA = auto()
    VALUE = auto()
    TRAILER = auto()


class Op(Enum):
    READ = auto()
    WRITE = auto()
    DECREMENT = auto()
    INCREMENT = auto()
    RESTORE = auto()
    TRANSFER = auto()

    # 结构性探针：用来表达「KeyA 不可读」，不属于访问位策略
    READ_KEY_A = auto()

    # 尾块的字段级操作（人类视图里看得见，机器视图里由条件表决定）
    WRITE_KEY_A = auto()
    READ_ACCESS = auto()
    WRITE_ACCESS = auto()
    READ_KEY_B = auto()
    WRITE_KEY_B = auto()


# ---------------------------------------------------------------- address

@dataclass(frozen=True, slots=True)
class Addr:
    """块地址。替代伪代码里裸传的 (sector, block) 两个 int。"""

    sector: int
    block: int

    def __post_init__(self) -> None:
        if not 0 <= self.sector < SECTORS_1K:
            raise ValueError(f"sector out of range: {self.sector}")
        if not 0 <= self.block < BLOCKS_PER_SECTOR:
            raise ValueError(f"block out of range: {self.block}")

    @property
    def flat(self) -> int:
        """拉平后的块号，0..63（1K 卡）。"""
        return self.sector * BLOCKS_PER_SECTOR + self.block


# ---------------------------------------------------------------- access tables

_A = frozenset({Auth.KEY_A})
_B = frozenset({Auth.KEY_B})
_AB = frozenset({Auth.KEY_A, Auth.KEY_B})

Condition = tuple[int, int, int]  # (C1, C2, C3)

# 数据块（block 0..2）的 8 行条件表。
# 001 是「只能减」的值块；110 是「只能加」的值块。
DATA_TABLE: Final[dict[Condition, dict[Op, frozenset[Auth]]]] = {
    (0, 0, 0): {
        Op.READ: _AB,
        Op.WRITE: _AB,
        Op.INCREMENT: _AB,
        Op.DECREMENT: _AB,
        Op.RESTORE: _AB,
        Op.TRANSFER: _AB,
    },
    (0, 1, 0): {Op.READ: _AB},
    (1, 0, 0): {Op.READ: _AB, Op.WRITE: _B},
    (1, 1, 0): {Op.READ: _AB, Op.WRITE: _B, Op.INCREMENT: _B},
    (0, 0, 1): {
        Op.READ: _AB,
        Op.DECREMENT: _AB,
        Op.RESTORE: _AB,
        Op.TRANSFER: _AB,
    },
    (0, 1, 1): {Op.READ: _B, Op.WRITE: _B},
    (1, 0, 1): {Op.READ: _B},
    (1, 1, 1): {},
}

# 尾块（block 3）的 8 行条件表。KeyA 永远不可读。
TRAILER_TABLE: Final[dict[Condition, dict[Op, frozenset[Auth]]]] = {
    (0, 0, 0): {
        Op.WRITE_KEY_A: _A,
        Op.READ_ACCESS: _A,
        Op.READ_KEY_B: _A,
        Op.WRITE_KEY_B: _A,
    },
    (0, 1, 0): {Op.READ_ACCESS: _A, Op.READ_KEY_B: _A},
    (1, 0, 0): {
        Op.WRITE_KEY_A: _B,
        Op.READ_ACCESS: _AB,
        Op.READ_KEY_B: _AB,
        Op.WRITE_KEY_B: _B,
    },
    (1, 1, 0): {Op.READ_ACCESS: _AB, Op.READ_KEY_B: _AB},
    (0, 0, 1): {
        Op.WRITE_KEY_A: _A,
        Op.READ_ACCESS: _A,
        Op.WRITE_ACCESS: _A,
        Op.READ_KEY_B: _A,
        Op.WRITE_KEY_B: _A,
    },
    (0, 1, 1): {
        Op.WRITE_KEY_A: _B,
        Op.READ_ACCESS: _AB,
        Op.WRITE_ACCESS: _B,
        Op.READ_KEY_B: _AB,
        Op.WRITE_KEY_B: _B,
    },
    (1, 0, 1): {Op.READ_ACCESS: _AB, Op.READ_KEY_B: _AB},
    (1, 1, 1): {Op.READ_ACCESS: _AB, Op.READ_KEY_B: _AB},
}


def encode_access(c1: int, c2: int, c3: int) -> bytes:
    """把 3 个 4-bit nibble 打包成真实的 3 字节访问位。

    每个 nibble 的 bit i 对应 block i。高 4 位存反相副本。
    """
    for name, value in (("c1", c1), ("c2", c2), ("c3", c3)):
        if not 0 <= value <= 0xF:
            raise ValueError(f"{name} must be a 4-bit nibble, got {value}")

    return bytes(
        (
            ((~c2 & 0xF) << 4) | (~c1 & 0xF),
            ((c1 & 0xF) << 4) | (~c3 & 0xF),
            ((c3 & 0xF) << 4) | (c2 & 0xF),
        )
    )


def _kind_for(block: int, condition: Condition) -> Kind:
    if block == 3:
        return Kind.TRAILER
    if condition in {(0, 0, 1), (1, 1, 0)}:
        return Kind.VALUE
    return Kind.DATA


# ---------------------------------------------------------------- access views

@dataclass(frozen=True, slots=True)
class BlockRule:
    """人类视图：一个块的权限规则，哪种钥匙能做哪些操作。"""

    kind: Kind
    key_a_ops: frozenset[Op] = frozenset()
    key_b_ops: frozenset[Op] = frozenset()

    def __post_init__(self) -> None:
        object.__setattr__(self, "key_a_ops", frozenset(self.key_a_ops))
        object.__setattr__(self, "key_b_ops", frozenset(self.key_b_ops))

    def allows(self, auth: Auth, op: Op) -> bool:
        match auth:
            case Auth.KEY_A:
                return op in self.key_a_ops
            case Auth.KEY_B:
                return op in self.key_b_ops
            case _:
                return False


def _rule_from_row(block: int, condition: Condition) -> BlockRule:
    table = TRAILER_TABLE if block == 3 else DATA_TABLE
    row = table[condition]
    return BlockRule(
        kind=_kind_for(block, condition),
        key_a_ops=frozenset(
            op for op, keys in row.items() if Auth.KEY_A in keys
        ),
        key_b_ops=frozenset(
            op for op, keys in row.items() if Auth.KEY_B in keys
        ),
    )


def _condition_for_rule(block: int, rule: BlockRule) -> Condition:
    table = TRAILER_TABLE if block == 3 else DATA_TABLE
    for condition in table:
        if _rule_from_row(block, condition) == rule:
            return condition
    raise UnencodableAccess(
        f"block {block}: no 3-bit condition matches {rule!r}"
    )


@dataclass(frozen=True, slots=True)
class AccessBits:
    """两种视图共存的访问位。

    raw 是唯一真相；rules 是它解码出来的人类视图。
    """

    raw: bytes = b"\xff\x03\xc0"

    def __post_init__(self) -> None:
        object.__setattr__(self, "raw", bytes(self.raw))
        if len(self.raw) != 3:
            raise ValueError(f"access bits must be 3 bytes, got {len(self.raw)}")
        self._decode_nibbles()  # 顺便校验反相冗余

    # ---- 机器视图 ----

    @classmethod
    def from_raw(cls, raw: bytes) -> "AccessBits":
        return cls(raw)

    def to_raw(self) -> bytes:
        return self.raw

    def _decode_nibbles(self) -> tuple[int, int, int]:
        raw = self.raw
        c1 = (raw[1] >> 4) & 0xF
        c2 = raw[2] & 0xF
        c3 = (raw[2] >> 4) & 0xF

        if (raw[0] & 0xF) != (~c1 & 0xF):
            raise ValueError("bad access bits: C1 redundant copy mismatch")
        if (raw[0] >> 4) != (~c2 & 0xF):
            raise ValueError("bad access bits: C2 redundant copy mismatch")
        if (raw[1] & 0xF) != (~c3 & 0xF):
            raise ValueError("bad access bits: C3 redundant copy mismatch")

        return c1, c2, c3

    def condition(self, block: int) -> Condition:
        if not 0 <= block < BLOCKS_PER_SECTOR:
            raise ValueError(f"block out of range: {block}")
        c1, c2, c3 = self._decode_nibbles()
        return ((c1 >> block) & 1, (c2 >> block) & 1, (c3 >> block) & 1)

    # ---- 人类视图 ----

    @property
    def rules(self) -> tuple[BlockRule, BlockRule, BlockRule, BlockRule]:
        return (
            _rule_from_row(0, self.condition(0)),
            _rule_from_row(1, self.condition(1)),
            _rule_from_row(2, self.condition(2)),
            _rule_from_row(3, self.condition(3)),
        )

    def to_rules(self) -> tuple[BlockRule, BlockRule, BlockRule, BlockRule]:
        return self.rules

    @classmethod
    def from_rules(
        cls, rules: tuple[BlockRule, BlockRule, BlockRule, BlockRule]
    ) -> "AccessBits":
        if len(rules) != BLOCKS_PER_SECTOR:
            raise ValueError("expected exactly 4 BlockRule entries")

        nibbles = [0, 0, 0]  # c1, c2, c3
        for block, rule in enumerate(rules):
            c1, c2, c3 = _condition_for_rule(block, rule)
            for index, bit in enumerate((c1, c2, c3)):
                nibbles[index] |= bit << block

        return cls(encode_access(*nibbles))

    # ---- 判定与展示 ----

    def kind_of(self, block: int) -> Kind:
        return _kind_for(block, self.condition(block))

    def decide(self, auth: Auth, op: Op, block: int) -> bool:
        table = TRAILER_TABLE if block == 3 else DATA_TABLE
        return auth in table[self.condition(block)].get(op, frozenset())

    def summary(self) -> str:
        lines = [f"raw = {self.raw.hex(' ')}"]
        for block, rule in enumerate(self.rules):
            key_a = ",".join(sorted(op.name for op in rule.key_a_ops)) or "-"
            key_b = ",".join(sorted(op.name for op in rule.key_b_ops)) or "-"
            lines.append(
                f"block {block}: {rule.kind.name:7s} "
                f"condition={self.condition(block)} "
                f"A[{key_a}] B[{key_b}]"
            )
        return "\n".join(lines)


def default_access() -> AccessBits:
    """一份可工作的默认策略，用人类视图构造。

    块 0/1: 000，数据块，A|B 全权限
    块 2  : 001，值块，A|B 可读/减/恢复/提交
    块 3  : 001，尾块，A 可写 KeyA、读写访问位和 KeyB
    """
    all_data = frozenset(
        {
            Op.READ,
            Op.WRITE,
            Op.INCREMENT,
            Op.DECREMENT,
            Op.RESTORE,
            Op.TRANSFER,
        }
    )
    value_ops = frozenset(
        {Op.READ, Op.DECREMENT, Op.RESTORE, Op.TRANSFER}
    )
    trailer_a = frozenset(
        {
            Op.WRITE_KEY_A,
            Op.READ_ACCESS,
            Op.WRITE_ACCESS,
            Op.READ_KEY_B,
            Op.WRITE_KEY_B,
        }
    )

    return AccessBits.from_rules(
        (
            BlockRule(Kind.DATA, all_data, all_data),
            BlockRule(Kind.DATA, all_data, all_data),
            BlockRule(Kind.VALUE, value_ops, value_ops),
            BlockRule(Kind.TRAILER, trailer_a, frozenset()),
        )
    )


# ---------------------------------------------------------------- storage

@dataclass(slots=True)
class Trailer:
    key_a: Key
    access: AccessBits
    key_b: Key

    def __post_init__(self) -> None:
        self.key_a = as_key(self.key_a)
        self.key_b = as_key(self.key_b)

    def to_block(self, mask_key_a: bool = True) -> Block:
        """把尾块编码成 16 字节。

        mask_key_a=True 时 KeyA 返回全 0：真实卡片上 KeyA 永远读不出来。
        访问位现在是真实编码，不再是占位 0。
        """
        key_a = b"\x00" * KEY_SIZE if mask_key_a else self.key_a
        return as_block(
            key_a + self.access.to_raw() + b"\x00" + self.key_b
        )


@dataclass(slots=True)
class Sector:
    data: list[Block] = field(
        default_factory=lambda: [as_block(bytes(BLOCK_SIZE)) for _ in range(3)]
    )
    trailer: Trailer = field(
        default_factory=lambda: Trailer(
            as_key(bytes(KEY_SIZE)), default_access(), as_key(bytes(KEY_SIZE))
        )
    )
    # transient：断电清零，不进入持久化镜像
    auth: Auth = field(default=Auth.NONE, metadata={"transient": True})

    def block_at(self, block: int) -> Block:
        if block == 3:
            return self.trailer.to_block(mask_key_a=True)
        if not 0 <= block < 3:
            raise IndexError(block)
        return self.data[block]

    def to_image(self) -> dict:
        return {
            "data": [bytes(b) for b in self.data],
            "trailer": self.trailer,
        }


@dataclass(slots=True)
class Card:
    sectors: list[Sector] = field(
        default_factory=lambda: [Sector() for _ in range(SECTORS_1K)]
    )
    # 整张卡只有一个传输缓冲，且断电即丢
    transfer: Optional[Block] = field(
        default=None, metadata={"transient": True}
    )

    def reset_session(self) -> None:
        """模拟离开射频场：所有易失状态清零。"""
        for sector in self.sectors:
            sector.auth = Auth.NONE
        self.transfer = None

    def to_image(self) -> dict:
        """持久化镜像：不包含 auth / transfer。"""
        return {"sectors": [s.to_image() for s in self.sectors]}


# ---------------------------------------------------------------- errors

class MifareError(Exception):
    pass


class AccessDenied(MifareError):
    pass


class InvalidOperation(MifareError):
    pass


class UnencodableAccess(MifareError):
    """人类视图无法用 3 bit 条件表表达。"""


# ---------------------------------------------------------------- core

def kind(card: Card, a: Addr) -> Kind:
    if a.block == 3:
        return Kind.TRAILER
    return card.sectors[a.sector].trailer.access.kind_of(a.block)


def block_at(card: Card, a: Addr) -> Block:
    return card.sectors[a.sector].block_at(a.block)


def allowed(card: Card, a: Addr, op: Op) -> bool:
    """两层裁决：结构规则优先，策略规则其次。"""
    sector = card.sectors[a.sector]

    # 结构规则：与访问位无关
    if sector.auth is Auth.NONE:
        return False
    if a.block == 3 and op is Op.READ_KEY_A:
        return False

    # 策略规则：由尾块里的访问位决定
    return sector.trailer.access.decide(sector.auth, op, a.block)


def authenticate(card: Card, sector: int, key_type: Auth, candidate: Key) -> None:
    if key_type not in (Auth.KEY_A, Auth.KEY_B):
        raise ValueError("key_type must be KEY_A or KEY_B")
    if not 0 <= sector < SECTORS_1K:
        raise ValueError(f"sector out of range: {sector}")

    s = card.sectors[sector]
    expected = s.trailer.key_a if key_type is Auth.KEY_A else s.trailer.key_b
    if candidate != expected:
        raise AccessDenied("authentication failed")

    s.auth = key_type  # 只影响这一个扇区


def _read_trailer(card: Card, a: Addr) -> Block:
    s = card.sectors[a.sector]
    if s.auth is Auth.NONE:
        raise AccessDenied(f"read denied at {a}")

    access = s.trailer.access
    can_access = access.decide(s.auth, Op.READ_ACCESS, 3)
    can_key_b = access.decide(s.auth, Op.READ_KEY_B, 3)

    if not can_access and not can_key_b:
        raise AccessDenied(f"trailer read denied at {a}")

    raw_access = access.to_raw() if can_access else b"\x00" * 3
    key_b = s.trailer.key_b if can_key_b else b"\x00" * KEY_SIZE
    return as_block(b"\x00" * KEY_SIZE + raw_access + b"\x00" + key_b)


def read_block(card: Card, a: Addr) -> Block:
    if a.block == 3:
        return _read_trailer(card, a)
    if not allowed(card, a, Op.READ):
        raise AccessDenied(f"read denied at {a}")
    return block_at(card, a)


def read_key_a(card: Card, a: Addr) -> Key:
    # 结构性规则：无论访问位怎么配，KeyA 都不可读
    raise AccessDenied("KeyA is write-only")


def _write_trailer(card: Card, a: Addr, data: Block) -> None:
    s = card.sectors[a.sector]
    if s.auth is Auth.NONE:
        raise AccessDenied(f"write denied at {a}")

    old = s.trailer
    new_key_a = as_key(data[0:6])
    try:
        new_access = AccessBits.from_raw(data[6:9])
    except ValueError as exc:
        raise InvalidOperation(str(exc)) from exc
    new_key_b = as_key(data[10:16])

    # 只校验真正发生变化的字段
    if new_key_a != old.key_a and not old.access.decide(
        s.auth, Op.WRITE_KEY_A, 3
    ):
        raise AccessDenied("KeyA write denied")
    if new_access != old.access and not old.access.decide(
        s.auth, Op.WRITE_ACCESS, 3
    ):
        raise AccessDenied("access bits write denied")
    if new_key_b != old.key_b and not old.access.decide(
        s.auth, Op.WRITE_KEY_B, 3
    ):
        raise AccessDenied("KeyB write denied")

    s.trailer = Trailer(new_key_a, new_access, new_key_b)


def write_block(card: Card, a: Addr, data: Block) -> None:
    data = as_block(data)
    if a.block == 3:
        _write_trailer(card, a, data)
        return

    if not allowed(card, a, Op.WRITE):
        raise AccessDenied(f"write denied at {a}")
    card.sectors[a.sector].data[a.block] = data


# ---------------------------------------------------------------- value blocks

@dataclass(frozen=True, slots=True)
class Value:
    value: int
    addr: int


def decode_value(block: Block) -> Value:
    value = int.from_bytes(block[0:4], "little")
    inverted = int.from_bytes(block[4:8], "little")
    again = int.from_bytes(block[8:12], "little")

    if value != (inverted ^ VALUE_MAX) or value != again:
        raise InvalidOperation("bad value block: value fields disagree")

    addr = block[12]
    if block[13] != (addr ^ 0xFF) or block[14] != addr or block[15] != (addr ^ 0xFF):
        raise InvalidOperation("bad value block: address fields disagree")

    return Value(value, addr)


def encode_value(value: int, a: Addr) -> Block:
    if not 0 <= value <= VALUE_MAX:
        raise ValueError("value out of range")

    raw = value.to_bytes(4, "little")
    inverted = (value ^ VALUE_MAX).to_bytes(4, "little")
    addr = a.flat
    return as_block(
        raw
        + inverted
        + raw
        + bytes([addr, addr ^ 0xFF, addr, addr ^ 0xFF])
    )


def value_op(card: Card, a: Addr, delta: int) -> None:
    op = Op.DECREMENT if delta < 0 else Op.INCREMENT
    if not allowed(card, a, op):
        raise AccessDenied(f"{op.name.lower()} denied at {a}")
    if kind(card, a) is not Kind.VALUE:
        raise InvalidOperation("not a value block")

    current = decode_value(block_at(card, a))
    new_value = current.value + delta
    if new_value < 0:
        raise AccessDenied("insufficient value")
    if new_value > VALUE_MAX:
        raise InvalidOperation("value overflow")

    # 只写暂存器，不落盘
    card.transfer = encode_value(new_value, a)


def restore(card: Card, a: Addr) -> None:
    if not allowed(card, a, Op.RESTORE):
        raise AccessDenied(f"restore denied at {a}")
    if kind(card, a) is not Kind.VALUE:
        raise InvalidOperation("not a value block")

    card.transfer = block_at(card, a)


def transfer(card: Card, a: Addr) -> None:
    if not allowed(card, a, Op.TRANSFER):
        raise AccessDenied(f"transfer denied at {a}")
    if kind(card, a) is not Kind.VALUE:
        raise InvalidOperation("not a value block")
    if card.transfer is None:
        raise InvalidOperation("no staged value to transfer")

    card.sectors[a.sector].data[a.block] = card.transfer
    card.transfer = None


# ---------------------------------------------------------------- commands

@dataclass(frozen=True, slots=True)
class AuthCmd:
    sector: int
    key_type: Auth
    key: Key


@dataclass(frozen=True, slots=True)
class ReadCmd:
    addr: Addr


@dataclass(frozen=True, slots=True)
class WriteCmd:
    addr: Addr
    data: Block


@dataclass(frozen=True, slots=True)
class ValueCmd:
    addr: Addr
    delta: int


@dataclass(frozen=True, slots=True)
class RestoreCmd:
    addr: Addr


@dataclass(frozen=True, slots=True)
class TransferCmd:
    addr: Addr


Command = AuthCmd | ReadCmd | WriteCmd | ValueCmd | RestoreCmd | TransferCmd


def dispatch(card: Card, cmd: Command) -> object | None:
    match cmd:
        case AuthCmd(sector=s, key_type=kt, key=k):
            return authenticate(card, s, kt, k)
        case ReadCmd(addr=a):
            return read_block(card, a)
        case WriteCmd(addr=a, data=d):
            return write_block(card, a, d)
        case ValueCmd(addr=a, delta=d):
            return value_op(card, a, d)
        case RestoreCmd(addr=a):
            return restore(card, a)
        case TransferCmd(addr=a):
            return transfer(card, a)
        case _:
            raise TypeError(f"unknown command: {cmd!r}")


# ---------------------------------------------------------------- demo

def demo_access_bits() -> None:
    print("=== access bits: two views ===")

    raw = b"\xff\x07\x80"  # 工厂默认
    ab = AccessBits.from_raw(raw)
    print(f"raw = {raw.hex(' ')}")
    print(f"conditions = {[ab.condition(i) for i in range(4)]}")
    print(ab.summary())

    assert ab.to_raw() == raw
    assert ab.condition(0) == (0, 0, 0)
    assert ab.condition(3) == (0, 0, 1)
    print("ok: raw -> rules -> raw round-trip")

    custom = AccessBits.from_raw(encode_access(0, 0, 0b1100))
    assert custom.to_raw() == b"\xff\x03\xc0"
    assert custom.condition(2) == (0, 0, 1)
    assert custom.kind_of(2) is Kind.VALUE
    assert AccessBits.from_rules(custom.rules).to_raw() == custom.to_raw()
    print("ok: human rules -> raw")

    try:
        AccessBits.from_raw(b"\xff\x07\x81")
    except ValueError as exc:
        print(f"ok: bad redundancy rejected ({exc})")

    try:
        AccessBits.from_rules(
            (
                BlockRule(Kind.DATA, frozenset({Op.READ, Op.WRITE}), frozenset({Op.READ})),
                BlockRule(Kind.DATA, frozenset({Op.READ}), frozenset({Op.READ})),
                BlockRule(Kind.DATA, frozenset({Op.READ}), frozenset({Op.READ})),
                BlockRule(Kind.TRAILER, frozenset({Op.READ_ACCESS}), frozenset()),
            )
        )
    except UnencodableAccess as exc:
        print(f"ok: unencodable human rule rejected ({exc})")


def demo() -> None:
    print("\n=== card behavior ===")
    card = Card()
    addr = Addr(1, 0)

    # 1. 未认证 -> 读被拒
    try:
        read_block(card, addr)
    except AccessDenied:
        print("ok: unauthenticated read denied")

    # 2. 错误密钥 -> 认证失败
    try:
        authenticate(card, 1, Auth.KEY_A, as_key(b"\xff" * KEY_SIZE))
    except AccessDenied:
        print("ok: wrong key rejected")

    # 3. 正确密钥（默认全 0）-> 只打开扇区 1
    authenticate(card, 1, Auth.KEY_A, as_key(bytes(KEY_SIZE)))
    print("ok: sector 1 authenticated with key A")

    # 4. 扇区 2 仍然是关闭的
    try:
        read_block(card, Addr(2, 0))
    except AccessDenied:
        print("ok: sector 2 still locked")

    # 5. 普通读写
    write_block(card, addr, as_block(b"\xaa" * BLOCK_SIZE))
    assert read_block(card, addr) == b"\xaa" * BLOCK_SIZE
    print("ok: data block read/write")

    # 6. 值块：减法是两阶段提交
    value_addr = Addr(1, 2)
    card.sectors[1].data[2] = encode_value(100, value_addr)

    value_op(card, value_addr, -30)
    assert decode_value(read_block(card, value_addr)).value == 100  # 还没提交
    transfer(card, value_addr)
    assert decode_value(read_block(card, value_addr)).value == 70
    print("ok: value op is staged until transfer")

    # 7. 读尾块：KeyA 被抹成 0，访问位是真实编码
    trailer_addr = Addr(1, 3)
    view = read_block(card, trailer_addr)
    assert view[0:6] == b"\x00" * 6
    assert view[6:9] == b"\xff\x03\xc0"
    print("ok: trailer read masks KeyA and returns real access bits")

    # 8. 写尾块：逐字段校验权限
    new_trailer = as_block(
        b"\x00" * 6
        + encode_access(0, 0, 0b1100)
        + b"\x00"
        + b"\x22" * KEY_SIZE
    )
    write_block(card, trailer_addr, new_trailer)
    assert read_block(card, trailer_addr)[10:16] == b"\x22" * KEY_SIZE
    print("ok: trailer write respects field permissions")

    # 9. KeyA 读不出来
    try:
        read_key_a(card, trailer_addr)
    except AccessDenied:
        print("ok: KeyA is write-only")

    # 10. 断电：auth 和 transfer 都清零
    card.reset_session()
    try:
        read_block(card, addr)
    except AccessDenied:
        print("ok: session state is volatile")

    # 11. 持久化镜像不含 transient 字段
    image = card.to_image()
    assert "auth" not in image["sectors"][0]
    assert "transfer" not in image
    print("ok: image excludes transient state")


if __name__ == "__main__":
    demo_access_bits()
    demo()
