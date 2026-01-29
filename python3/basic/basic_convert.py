# /// script
# requires-python = ">=3.12"
# dependencies = [
#     "pysnooper",
# ]
# ///
import pysnooper

@pysnooper.snoop()
def int_to():
    res = int.to_bytes(0)
    res = int.to_bytes(0xDEADBEEF, 4, 'big')
    res = int.to_bytes(0xDEADBEEF, 4, 'little')


def main() -> None:
    int_to()


if __name__ == "__main__":
    main()
