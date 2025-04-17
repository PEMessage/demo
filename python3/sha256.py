import struct
import binascii
import hashlib
import unittest

def sha256(message):
    """
    SHA-256 哈希函数实现
    参数: message - 要哈希的字节串(bytes)
    返回: 64字节的哈希值(二进制格式)
    
    标准参考: FIPS PUB 180-4 (Federal Information Processing Standards Publication)
    https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.180-4.pdf
    """
    
    # 初始化哈希值 (前8个质数的平方根的小数部分前32位)
    # 标准参考: 5.3.3 SHA-256的初始哈希值
    h0 = 0x6a09e667
    h1 = 0xbb67ae85
    h2 = 0x3c6ef372
    h3 = 0xa54ff53a
    h4 = 0x510e527f
    h5 = 0x9b05688c
    h6 = 0x1f83d9ab
    h7 = 0x5be0cd19
    
    # 初始化轮常数 (前64个质数的立方根的小数部分前32位)
    # 标准参考: 4.2.2 SHA-256常量
    k = [
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
    ]
    
    # 预处理: 消息填充 (标准参考: 5.1.1 填充)
    original_length = len(message)  # 原始消息长度(字节)
    original_bit_length = original_length * 8  # 原始消息长度(位)
    
    # 1. 添加一个'1' bit (后面跟着7个'0' bits构成一个字节0x80)
    message += b'\x80'
    
    # 2. 填充'0' bits直到消息长度 ≡ 448 (mod 512)
    # 计算需要填充的字节数: (56 - (len + 1) % 64) % 64
    message += b'\x00' * ((56 - (original_length + 1) % 64) % 64)
    
    # 3. 添加原始消息长度的64位表示(大端序)
    message += struct.pack('>Q', original_bit_length)
    
    # 处理512-bit(64字节)的消息块 (标准参考: 6.2.2 SHA-256处理)
    for i in range(0, len(message), 64):
        chunk = message[i:i+64]
        
        # 将块分解为16个32-bit大端序字
        w = list(struct.unpack('>16L', chunk))
        
        # 消息调度: 将16个字扩展为64个字 (标准参考: 6.2.2步骤1)
        for j in range(16, 64):
            # σ0函数 (标准参考: 4.1.2)
            s0 = (right_rotate(w[j-15], 7) ^ right_rotate(w[j-15], 18) ^ (w[j-15] >> 3))
            # σ1函数
            s1 = (right_rotate(w[j-2], 17) ^ right_rotate(w[j-2], 19) ^ (w[j-2] >> 10))
            # 模2^32加法
            w.append((w[j-16] + s0 + w[j-7] + s1) & 0xffffffff)
        
        # 初始化工作变量为当前哈希值
        a, b, c, d, e, f, g, h = h0, h1, h2, h3, h4, h5, h6, h7
        
        # 压缩函数主循环 (标准参考: 6.2.2步骤2)
        for j in range(64):
            # Σ1函数 (标准参考: 4.1.2)
            S1 = (right_rotate(e, 6) ^ right_rotate(e, 11) ^ right_rotate(e, 25))
            # Ch函数 (选择函数)
            ch = (e & f) ^ ((~e) & g)
            # 临时变量temp1
            temp1 = (h + S1 + ch + k[j] + w[j]) & 0xffffffff
            # Σ0函数
            S0 = (right_rotate(a, 2) ^ right_rotate(a, 13) ^ right_rotate(a, 22))
            # Maj函数 (多数函数)
            maj = (a & b) ^ (a & c) ^ (b & c)
            # 临时变量temp2
            temp2 = (S0 + maj) & 0xffffffff
            
            # 更新工作变量 (标准参考: 6.2.2步骤3)
            h = g
            g = f
            f = e
            e = (d + temp1) & 0xffffffff
            d = c
            c = b
            b = a
            a = (temp1 + temp2) & 0xffffffff
        
        # 将压缩后的块添加到当前哈希值 (标准参考: 6.2.2步骤4)
        h0 = (h0 + a) & 0xffffffff
        h1 = (h1 + b) & 0xffffffff
        h2 = (h2 + c) & 0xffffffff
        h3 = (h3 + d) & 0xffffffff
        h4 = (h4 + e) & 0xffffffff
        h5 = (h5 + f) & 0xffffffff
        h6 = (h6 + g) & 0xffffffff
        h7 = (h7 + h) & 0xffffffff
    
    # 生成最终的哈希值(大端序) (标准参考: 6.2.2步骤5)
    return struct.pack('>8L', h0, h1, h2, h3, h4, h5, h6, h7)

def right_rotate(n, b):
    """32-bit数的右旋转操作"""
    return ((n >> b) | (n << (32 - b))) & 0xffffffff

def sha256_string(message):
    """计算字符串的SHA-256哈希值(返回16进制字符串)"""
    if isinstance(message, str):
        message = message.encode('utf-8')
    return binascii.hexlify(sha256(message)).decode('utf-8')



class TestSHA256(unittest.TestCase):
    """测试自定义SHA-256实现的测试用例"""
    
    def test_empty_string(self):
        """测试空字符串"""
        self.assertEqual(sha256_string(""), hashlib.sha256().hexdigest())
    
    def test_short_strings(self):
        """测试短字符串"""
        test_cases = [
            "a",
            "abc",
            "message digest",
            "abcdefghijklmnopqrstuvwxyz",
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789",
            "12345678901234567890123456789012345678901234567890123456789012345678901234567890"
        ]
        for s in test_cases:
            with self.subTest(msg=s):
                self.assertEqual(sha256_string(s), hashlib.sha256(s.encode()).hexdigest())
    
    def test_long_strings(self):
        """测试长字符串(超过单个块512-bit)"""
        long_str1 = "a" * 1000  # 1000个a
        long_str2 = "hello world " * 100  # 1200个字符
        self.assertEqual(sha256_string(long_str1), hashlib.sha256(long_str1.encode()).hexdigest())
        self.assertEqual(sha256_string(long_str2), hashlib.sha256(long_str2.encode()).hexdigest())
    
    def test_special_chars(self):
        """测试特殊字符"""
        cases = [
            " ", "\t", "\n", "\r\n",
            "!@#$%^&*()",
            "中文测试",
            "🍎🐶",  # Unicode符号
        ]
        for s in cases:
            with self.subTest(msg=s):
                self.assertEqual(sha256_string(s), hashlib.sha256(s.encode('utf-8')).hexdigest())
    
    def test_binary_data(self):
        """测试二进制数据"""
        binary_data = bytes(range(256))  # 0-255的所有字节值
        self.assertEqual(
            sha256(binary_data).hex(),
            hashlib.sha256(binary_data).hexdigest()
        )
    
    def test_known_vectors(self):
        """测试标准测试向量(来自NIST示例)"""
        vectors = [
            # (输入消息, 预期SHA-256哈希值)
            ("", "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"),
            ("abc", "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"),
            ("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", 
             "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1"),
            ("The quick brown fox jumps over the lazy dog",
             "d7a8fbb307d7809469ca9abcb0082e4f8d5651e46d3cdb762d02d0bf37c9e592"),
            ("The quick brown fox jumps over the lazy cog",  # 1字符差异
             "e4c4d8f3bf76b692de791a173e05321150f7a345b46484fe427f6acc7ecc81be")
        ]
        for msg, expected in vectors:
            with self.subTest(msg=msg):
                self.assertEqual(sha256_string(msg), expected)
    
    def test_edge_cases(self):
        """测试边界条件"""
        # 刚好达到块边界的情况
        boundary1 = "a" * 55  # 55字节: 55 + 1(0x80) + 8(长度) = 64
        boundary2 = "a" * 56  # 56字节: 56 + 1 + 7(填充) + 8 = 72 → 需要两个块
        self.assertEqual(sha256_string(boundary1), hashlib.sha256(boundary1.encode()).hexdigest())
        self.assertEqual(sha256_string(boundary2), hashlib.sha256(boundary2.encode()).hexdigest())

if __name__ == "__main__":
    unittest.main()
