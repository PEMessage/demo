# See: https://www.bilibili.com/video/BV1iu2mByEXB?t=87.4

def encrypto(message, key):
    return ''.join([chr(ord(x) ^ key) for x in message])


message = "Hello, World"
print(message)

cipher = encrypto(message, 77)
print(cipher)

decrypted = encrypto(cipher, 77)
print(decrypted)
