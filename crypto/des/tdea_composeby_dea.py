# /// script
# requires-python = ">=3.12"
# dependencies = [
#     "pycryptodome",
# ]
# ///

from Crypto.Cipher import DES, DES3
from Crypto.Util.Padding import pad, unpad
import binascii

BLOCK_SIZE = 8
KEY_16BYTE = b'12345678abcdefgh'
PLAINTEXT = b'HelloTDEA'


# X9.24-2017 provide this short and brief top-view of how DEA compose to TDEA
# P67 C3.7:
#   C.3.7. "Triple-DEA Encrypt" (Local Subroutine)
#     kkk) Crypto Register-1 DEA-encrypted using the left half of the Key Register as the key goes to Crypto Register-1
#     Ill) Crypto Register-1 DEA-decrypted using the right half of the Keey Register as the key goes to Crypto Register-1
#     mmm) Crypto Register-1 DEA-encrypted using the left half of tthe Key Register as the key goes to Crypto Register-1
#     nnn) Return from subroutine
# Question:
#   TDES keylen is 8*2 ??? not 8*3=24 ???
def manual_tdea_ede_encrypt(plaintext, key_16byte):
    k1 = key_16byte[:8]
    k2 = key_16byte[8:]

    # 步骤1：明文填充（PKCS7 填充，与 pycryptodome 默认一致）
    padded_plain = pad(plaintext, BLOCK_SIZE)

    # 步骤2：第一次 DES 加密（E，用 K1）
    des1 = DES.new(k1, DES.MODE_ECB)  # ECB 模式（TDEA 基础是块操作，ECB 是基础）
    step1 = des1.encrypt(padded_plain)

    # 步骤3：DES 解密（D，用 K2）
    des2 = DES.new(k2, DES.MODE_ECB)
    step2 = des2.decrypt(step1)

    # 步骤4：第二次 DES 加密（E，用 K1）
    step3 = des1.encrypt(step2)

    return step3


def manual_tdea_ede_decrypt(ciphertext, key_16byte):
    """
    TDEA E-D-E Decryption(which will be D-E-D)
    """
    k1 = key_16byte[:8]
    k2 = key_16byte[8:]

    # 步骤1：DES 解密（D，用 K1）
    des1 = DES.new(k1, DES.MODE_ECB)
    step1 = des1.decrypt(ciphertext)

    # 步骤2：DES 加密（E，用 K2）
    des2 = DES.new(k2, DES.MODE_ECB)
    step2 = des2.encrypt(step1)

    # 步骤3：DES 解密（D，用 K1）
    step3 = des1.decrypt(step2)

    # 去除填充
    return unpad(step3, BLOCK_SIZE)


def pycrypto_tdea_encrypt(plaintext, key_16byte):
    tdea = DES3.new(key_16byte, DES3.MODE_ECB)
    padded_plain = pad(plaintext, BLOCK_SIZE)
    ciphertext = tdea.encrypt(padded_plain)
    return ciphertext


def pycrypto_tdea_decrypt(ciphertext, key_16byte):
    tdea = DES3.new(key_16byte, DES3.MODE_ECB)
    decrypted = tdea.decrypt(ciphertext)
    return unpad(decrypted, BLOCK_SIZE)


if __name__ == "__main__":
    manual_cipher = manual_tdea_ede_encrypt(PLAINTEXT, KEY_16BYTE)
    manual_decrypt = manual_tdea_ede_decrypt(manual_cipher, KEY_16BYTE)

    pycrypto_cipher = pycrypto_tdea_encrypt(PLAINTEXT, KEY_16BYTE)
    pycrypto_decrypt = pycrypto_tdea_decrypt(pycrypto_cipher, KEY_16BYTE)

    # 1. Compare output
    print("=== Original Data ===")
    print(f"Plaintext: {PLAINTEXT}")
    print(f"Key (16 bytes): {KEY_16BYTE}")

    print("\n=== Encryption Result Comparison (Hexadecimal) ===")
    print(f"Manual TDEA Ciphertext: {manual_cipher.hex()}")
    print(f"pycryptodome TDEA Ciphertext: {pycrypto_cipher.hex()}")
    print(f"Ciphertext Match: {manual_cipher == pycrypto_cipher}")

    print("\n=== Decryption Result Comparison ===")
    print(f"Manual TDEA Decryption Result: {manual_decrypt}")
    print(f"pycryptodome TDEA Decryption Result: {pycrypto_decrypt}")
    print(f"Decryption Result Match: {manual_decrypt == pycrypto_decrypt}")

