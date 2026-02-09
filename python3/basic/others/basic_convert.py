# /// script
# requires-python = ">=3.12"
# dependencies = [
#     "pysnooper",
# ]
# ///
import pysnooper


# ===============================
# convert
# ===============================

@pysnooper.snoop()
def int_to():
    # -> to bytes
    res = int.to_bytes(0)
    res = int.to_bytes(0xDEADBEEF, 4, 'big')
    res = int.to_bytes(0xDEADBEEF, 4, 'little')

@pysnooper.snoop()
def byte_to():
    # -> hex string
    res = b'\x11\x22\xDE'.hex().upper()


@pysnooper.snoop()
def string_to():
    # -> raw byte
    res = "random text is fine".encode(encoding='ascii')






def main() -> None:
    int_to()
    byte_to()
    string_to()


if __name__ == "__main__":
    main()
