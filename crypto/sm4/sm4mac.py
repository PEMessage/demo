# /// script
# requires-python = ">=3.12"
# dependencies = [
#     "cryptography",
# ]
# ///

from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.backends import default_backend


# TODO: will Cipher padding for us? default padding with zero?
def sm4mac_without_padding(key: bytes, data: bytes) -> bytes:
    if len(key) != 16:
        raise ValueError("SM4 key must be 16 bytes")

    iv = b'\x00' * 16
    cipher = Cipher(algorithms.SM4(key), modes.CBC(iv), backend=default_backend())
    encryptor = cipher.encryptor()

    encrypted_data = encryptor.update(data) + encryptor.finalize()
    return encrypted_data[-16:]

def main() -> None:
    print("Hello from sm4mac.py!")

    key = bytes.fromhex('11' * 16)  # 16-byte key
    data = bytes.fromhex('0000000000000000111111111111111122222222222222223333333333333333')

    print(f"Key: {key.hex()}")
    print(f"Data: {data.hex()}")
    print(f"SM4 MAC: {sm4mac_without_padding(key, data).hex()}")


if __name__ == "__main__":
    main()
