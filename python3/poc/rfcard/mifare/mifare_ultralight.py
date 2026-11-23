"""MIFARE Ultralight —— Step 0~4 的行为模型。

覆盖的结构（以 MF0ICU1 为骨架）:

    page 0-1 : UID（出厂固化，只读）
    page 2   : 内部数据 + 锁字节
    page 3   : OTP（4 字节 = 32 个一次性位）
    page 4-15: 用户数据

模型做了三处刻意的简化，写在这里避免误读：

1. 锁位的「哪一位管哪一组页」用的是本模型自己的映射，
   不是芯片 datasheet 的精确位分配。
2. 认证是 EV1 风格的「密码 + PACK」，不是 Ultralight C 的 3DES。
3. 计数器是把 OTP 位当温度计用的视图，不是 EV1 那颗专用单向计数器。

还有一条容易混淆的边界：认证（password）只保护计数器的递减；
用户页的写保护由锁位负责。这是两条正交的线，Ultralight 没有
「每页一个密码」这种概念。

类型映射（伪代码 -> Python）:
    Byte[4]                  -> NewType("Page", bytes)
    struct OtpCounter        -> @dataclass(frozen=True) OtpCounter
    enum Auth                -> bool（Ultralight 只有全局一个开关）
    DENIED                   -> AccessDenied
    INVALID                  -> InvalidOperation
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Final, NewType

PAGE_SIZE: Final = 4
PAGES: Final = 16
UID_PAGES: Final = frozenset({0, 1})
LOCK_PAGE: Final = 2
OTP_PAGE: Final = 3
USER_FIRST: Final = 4

PASSWORD_SIZE: Final = 4
PACK_SIZE: Final = 2

# page 2 的 byte 2 是 LOCK0。这里用模型自己的分组：
#   bit 0..5 -> page 4-5, 6-7, 8-9, 10-11, 12-13, 14-15
#   bit 6    -> 锁字节自身（lock-lock）
#   bit 7    -> OTP 页
LOCK_GROUPS: Final = 6
LOCK_LOCK_BIT: Final = 6
OTP_LOCK_BIT: Final = 7

Page = NewType("Page", bytes)
Password = NewType("Password", bytes)
Pack = NewType("Pack", bytes)


def as_page(raw: bytes) -> Page:
    if len(raw) != PAGE_SIZE:
        raise ValueError(f"page must be {PAGE_SIZE} bytes, got {len(raw)}")
    return Page(bytes(raw))


def as_password(raw: bytes) -> Password:
    if len(raw) != PASSWORD_SIZE:
        raise ValueError(
            f"password must be {PASSWORD_SIZE} bytes, got {len(raw)}"
        )
    return Password(bytes(raw))


def as_pack(raw: bytes) -> Pack:
    if len(raw) != PACK_SIZE:
        raise ValueError(f"PACK must be {PACK_SIZE} bytes, got {len(raw)}")
    return Pack(bytes(raw))


# ---------------------------------------------------------------- storage

@dataclass(slots=True)
class UltralightCard:
    pages: list[Page] = field(
        default_factory=lambda: [
            as_page(bytes(PAGE_SIZE)) for _ in range(PAGES)
        ]
    )
    # EV1 的配置页在 16 页之外；模型把它收成两个持久化字段
    password: Password = field(default_factory=lambda: as_password(bytes(4)))
    pack: Pack = field(default_factory=lambda: as_pack(bytes(2)))
    # 全局认证状态：易失，断电清零
    authed: bool = field(default=False, metadata={"transient": True})

    def reset_session(self) -> None:
        """模拟离开射频场：认证状态清零。"""
        self.authed = False

    def to_image(self) -> dict:
        """持久化镜像：不包含 authed。"""
        return {
            "pages": [bytes(p) for p in self.pages],
            "password": self.password,
            "pack": self.pack,
        }


# ---------------------------------------------------------------- errors

class UltralightError(Exception):
    pass


class AccessDenied(UltralightError):
    pass


class InvalidOperation(UltralightError):
    pass


# ---------------------------------------------------------------- lock bytes

def _lock0(card: UltralightCard) -> int:
    return card.pages[LOCK_PAGE][2]


def _is_locked(card: UltralightCard, page: int) -> bool:
    if USER_FIRST <= page < PAGES:
        group = (page - USER_FIRST) // 2
        return bool(_lock0(card) & (1 << group))
    return False


def _lock_bytes_frozen(card: UltralightCard) -> bool:
    return bool(_lock0(card) & (1 << LOCK_LOCK_BIT))


def _otp_locked(card: UltralightCard) -> bool:
    return bool(_lock0(card) & (1 << OTP_LOCK_BIT))


def _burn_lock_bytes(card: UltralightCard, data: Page) -> None:
    """写 page 2：只能置位，不能清零；内部字节保持不变。"""
    if _lock_bytes_frozen(card):
        raise AccessDenied("lock bytes are frozen")

    current = card.pages[LOCK_PAGE]
    lock0 = current[2] | data[2]
    lock1 = current[3] | data[3]
    card.pages[LOCK_PAGE] = as_page(
        current[:2] + bytes([lock0, lock1])
    )


def _burn_bits(card: UltralightCard, page: int, data: Page) -> None:
    """OTP 写：只把 1 位烧上去，0 位不产生任何效果。"""
    if page == OTP_PAGE and _otp_locked(card):
        raise AccessDenied("OTP page is locked")

    current = card.pages[page]
    card.pages[page] = as_page(
        bytes(a | b for a, b in zip(current, data))
    )


# ---------------------------------------------------------------- page I/O

def read_page(card: UltralightCard, page: int) -> Page:
    if not 0 <= page < PAGES:
        raise ValueError(f"page out of range: {page}")
    return card.pages[page]


def write_page(card: UltralightCard, page: int, data: Page) -> None:
    data = as_page(data)
    if not 0 <= page < PAGES:
        raise ValueError(f"page out of range: {page}")

    if page in UID_PAGES:
        raise AccessDenied("UID pages are read-only")

    if page == LOCK_PAGE:
        _burn_lock_bytes(card, data)
        return

    if page == OTP_PAGE:
        _burn_bits(card, OTP_PAGE, data)
        return

    if _is_locked(card, page):
        raise AccessDenied(f"page {page} is locked")

    card.pages[page] = data


# ---------------------------------------------------------------- locking

def lock_group(card: UltralightCard, group: int) -> None:
    """锁住一组用户页：group 0 -> page 4-5，group 5 -> page 14-15。"""
    if not 0 <= group < LOCK_GROUPS:
        raise ValueError(f"group out of range: {group}")

    data = bytearray(PAGE_SIZE)
    data[2] = 1 << group
    write_page(card, LOCK_PAGE, as_page(bytes(data)))


def freeze_lock_bytes(card: UltralightCard) -> None:
    """把锁字节自身也锁住：之后 page 2 不可再写。"""
    data = bytearray(PAGE_SIZE)
    data[2] = 1 << LOCK_LOCK_BIT
    write_page(card, LOCK_PAGE, as_page(bytes(data)))


def lock_otp(card: UltralightCard) -> None:
    """锁住 OTP 页：之后 page 3 不可再烧。"""
    data = bytearray(PAGE_SIZE)
    data[2] = 1 << OTP_LOCK_BIT
    write_page(card, LOCK_PAGE, as_page(bytes(data)))


# ---------------------------------------------------------------- auth

def authenticate(card: UltralightCard, password: Password) -> Pack:
    """EV1 风格：密码正确则返回 PACK，并把全局 authed 置位。"""
    password = as_password(password)
    if password != card.password:
        raise AccessDenied("wrong password")
    card.authed = True
    return card.pack


# ---------------------------------------------------------------- OTP counter

@dataclass(frozen=True, slots=True)
class OtpCounter:
    """OTP 页上的一段位，按温度计码解释成一个只能减的计数器。"""

    start: int
    end: int
    page: int = OTP_PAGE

    def __post_init__(self) -> None:
        if self.page != OTP_PAGE:
            raise ValueError("this model only supports counters on the OTP page")
        if not 0 <= self.start <= self.end <= 31:
            raise ValueError("counter bit range must be within 0..31")

    @property
    def capacity(self) -> int:
        return self.end - self.start + 1


def _get_bit(page: Page, index: int) -> int:
    return (page[index // 8] >> (index % 8)) & 1


def _with_bit_set(page: Page, index: int) -> Page:
    raw = bytearray(page)
    raw[index // 8] |= 1 << (index % 8)
    return as_page(bytes(raw))


def remaining(card: UltralightCard, counter: OtpCounter) -> int:
    """还剩几次：范围内还没烧的位数。读是公开的，不需要认证。"""
    return sum(
        1
        for i in range(counter.start, counter.end + 1)
        if _get_bit(card.pages[counter.page], i) == 0
    )


def decrement(card: UltralightCard, counter: OtpCounter) -> None:
    """烧掉范围内最低的一个 0 位；需要已认证。"""
    if not card.authed:
        raise AccessDenied("not authenticated")
    if _otp_locked(card):
        raise AccessDenied("OTP page is locked")

    for i in range(counter.start, counter.end + 1):
        if _get_bit(card.pages[counter.page], i) == 0:
            card.pages[counter.page] = _with_bit_set(
                card.pages[counter.page], i
            )
            return

    raise AccessDenied("counter exhausted")


# ---------------------------------------------------------------- demo

def demo() -> None:
    card = UltralightCard()

    # 1. UID 只读
    print("uid:", read_page(card, 0).hex(), read_page(card, 1).hex())
    try:
        write_page(card, 0, as_page(b"\x00" * PAGE_SIZE))
    except AccessDenied:
        print("ok: UID is read-only")

    # 2. 用户页可写
    write_page(card, 4, as_page(b"\xde\xad\xbe\xef"))
    assert read_page(card, 4) == b"\xde\xad\xbe\xef"
    print("ok: user page write")

    # 3. 锁住 group 0（page 4-5），group 1 仍然可写
    lock_group(card, 0)
    try:
        write_page(card, 4, as_page(b"\x00" * PAGE_SIZE))
    except AccessDenied:
        print("ok: locked page rejects write")
    write_page(card, 6, as_page(b"\x01\x02\x03\x04"))
    print("ok: unlocked group still writable")

    # 4. OTP 只能 0 -> 1
    write_page(card, OTP_PAGE, as_page(b"\x01\x00\x00\x00"))
    assert _get_bit(read_page(card, OTP_PAGE), 0) == 1
    write_page(card, OTP_PAGE, as_page(b"\x00\x00\x00\x00"))
    assert _get_bit(read_page(card, OTP_PAGE), 0) == 1
    print("ok: OTP bit cannot be cleared")

    # 5. 计数器：bit 8..11，容量 4
    counter = OtpCounter(start=8, end=11)
    assert remaining(card, counter) == 4

    try:
        decrement(card, counter)
    except AccessDenied:
        print("ok: decrement needs authentication")

    try:
        authenticate(card, as_password(b"\xff" * PASSWORD_SIZE))
    except AccessDenied:
        print("ok: wrong password rejected")

    pack = authenticate(card, as_password(bytes(PASSWORD_SIZE)))
    print("ok: authenticated, PACK =", pack.hex())

    decrement(card, counter)
    assert remaining(card, counter) == 3
    print("ok: counter decremented to", remaining(card, counter))

    # 6. 认证是易失的
    card.reset_session()
    try:
        decrement(card, counter)
    except AccessDenied:
        print("ok: authentication is volatile")

    # 7. 锁字节自身也能冻住
    freeze_lock_bytes(card)
    try:
        write_page(card, LOCK_PAGE, as_page(bytes(PAGE_SIZE)))
    except AccessDenied:
        print("ok: lock bytes are frozen")

    # 8. OTP 页也能单独锁住
    other = UltralightCard()
    lock_otp(other)
    try:
        write_page(other, OTP_PAGE, as_page(b"\x01\x00\x00\x00"))
    except AccessDenied:
        print("ok: OTP page can be locked")

    # 9. 持久化镜像不含 transient 字段
    image = card.to_image()
    assert "authed" not in image
    print("ok: image excludes transient state")


if __name__ == "__main__":
    demo()
