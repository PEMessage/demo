# /// script
# requires-python = ">=3.12"
# dependencies = [
#     "cryptography",
# ]
# ///

from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives import padding



# If you set (begin_padder == repeat_padder == 0x10), you get PKCS7
# If you set (begin_padder : 0x80, repeat_padder : 0x00), you get ISO
# If you set (begin_padder : '', repeat_padder : 0x00), you get fill_to block_size
def gen_padding(
    data: bytes = b'',
    begin_padder: bytes = b'',
    repeat_padder: bytes = b'\x00',
    block_size: int = 16
) -> bytes:
    if len(repeat_padder) != 1:
        raise ValueError("Padder must be a single byte")

    data = data + begin_padder

    data_len = len(data)
    remainder = data_len % block_size
    padding_len = (block_size - remainder) % block_size
    return data + (repeat_padder * padding_len)

def gen_pkcs7_like_padding(padder):
    def func(data):
        return gen_padding(
            data,
            begin_padder=padder,
            repeat_padder=padder
        )
    return func

def gen_iso_like_padding(begin_padder, repeat_padder):
    def func(data):
        return gen_padding(
            data,
            begin_padder=begin_padder,
            repeat_padder=repeat_padder
        )
    return func

def gen_fill(padder):
    def func(data):
        return gen_padding(
            data,
            begin_padder=b'',
            repeat_padder=padder
        )
    return func



def pkcs7_padding(data: bytes) -> bytes:
    padder = padding.PKCS7(128).padder()
    return padder.update(data) + padder.finalize()

def dummy_padding(data: bytes) -> bytes:
    return data


def sm4mac(key: bytes, data: bytes, iv: bytes = b'\x00' * 16, padding_methoh = dummy_padding):
    if len(key) != 16:
        raise ValueError("SM4 key must be 16 bytes")

    padded_data = padding_methoh(data)
    print(f"Padded Data: {padded_data.hex()}")

    cipher = Cipher(algorithms.SM4(key), modes.CBC(iv), backend=default_backend())
    encryptor = cipher.encryptor()

    encrypted_data = encryptor.update(padded_data) + encryptor.finalize()
    return encrypted_data[-16:]

def main() -> None:
    print("Hello from sm4mac.py!")

    key = bytes.fromhex('11' * 16)  # 16-byte key
    data = bytes.fromhex('0000000000000000111111111111111122222222222222223333333333333333')
    print("============================")
    print(f"Key: {key.hex()}")
    print(f"Data: {data.hex()}")

    print("==============")
    print(f"SM4 MAC: {sm4mac(key, data).hex()}")
    print("==============")
    print(f"SM4 PKCS MAC: {sm4mac(key, data, padding_methoh=pkcs7_padding).hex()}")

    for i in [
        0x00,
        0x10, # same as PKCS7
    ]:
        padder = int.to_bytes(i)
        print("==============")
        print(f"SM4 Padder {padder.hex()} MAC: {sm4mac(key, data, padding_methoh=gen_pkcs7_like_padding(padder)).hex()}")

    for begin_padder, repeat_padder in [
        (0x80, 0x00) # ISO/IEC 9797-1 padding
    ]:
        begin_padder = int.to_bytes(begin_padder)
        repeat_padder = int.to_bytes(repeat_padder)
        print("==============")
        print(f"SM4 ISO Padder MAC: {sm4mac(key, data, padding_methoh=gen_iso_like_padding(begin_padder, repeat_padder)).hex()}")

    sm4ecb(key, data)



if __name__ == "__main__":
    main()
