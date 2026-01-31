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

def gen_unconditional_padding(padder):
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

def xor_by_blocks(data: bytes, block_size: int) -> bytes:
    if len(data) % block_size != 0:
        raise ValueError(f"len(data) must be modable by block_size: {block_size}")

    result = bytearray(block_size)
    num_blocks = len(data) // block_size

    for i in range(num_blocks):
        block_start = i * block_size
        block_end = block_start + block_size
        block = data[block_start:block_end]

        for j in range(block_size):
            result[j] ^= block[j]

    return bytes(result)




def main(key, data) -> None:
    print("----------------------------")
    print(f"Key: {key.hex()}")
    print(f"Data: {data.hex()}")
    print("----------------------------")

    print("==============")
    print(f"SM4 PKCS MAC: {sm4mac(key, data, padding_methoh=pkcs7_padding).hex()}")

    for i in [
        0x00
    ]:
        padder = int.to_bytes(i)
        print("==============")
        print(f"SM4 dummy fill {padder.hex()} MAC: {sm4mac(key, data, padding_methoh=gen_fill(padder)).hex()}")


    for i in [
        0x00,
        0x10,
    ]:
        padder = int.to_bytes(i)
        print("==============")
        print(f"SM4 Uncondition Padder {padder.hex()} MAC: {sm4mac(key, data, padding_methoh=gen_unconditional_padding(padder)).hex()}")

    for begin_padder, repeat_padder in [
        (0x80, 0x00) # ISO/IEC 9797-1 padding
    ]:
        begin_padder = int.to_bytes(begin_padder)
        repeat_padder = int.to_bytes(repeat_padder)
        print("==============")
        print(f"SM4 ISO Padder MAC: {sm4mac(key, data, padding_methoh=gen_iso_like_padding(begin_padder, repeat_padder)).hex()}")



if __name__ == "__main__":
    keys = [
        bytes.fromhex('11' * 16),
        bytes.fromhex('01' * 16),
        ('1' * 16).encode()
    ]
    datas = [
        bytes.fromhex('0000000000000000111111111111111122222222222222223333333333333333'),
        bytes.fromhex('22' * 16),
        bytes.fromhex('22' * 8),
        bytes.fromhex('22' * 4),
        bytes.fromhex('02' * 16),
        bytes.fromhex('02' * 8),
        bytes.fromhex('02' * 4),
        ('2' * 16).encode(),
        ('2' * 8).encode(),
        ('2' * 4).encode(),
    ]

    for key in keys:
        for data in datas:
            main(key, data)
