"""Tests for ISO/IEC 9797-1 MAC algorithms using test vectors from Annex B."""

import pytest

from mac_cli import (
    MACAlgorithm,
    KeyDerivationMethod,
    PaddingMethod,
    calculate_mac,
    mac_algorithm_1,
    mac_algorithm_2,
    mac_algorithm_3,
    mac_algorithm_5,
    pad,
    unpad,
    derive_keys_kdm2,
    multx,
)


class TestPaddingMethods:
    """Test ISO/IEC 9797-1 padding methods."""

    def test_padding_method_1(self):
        """Test Padding Method 1: right-pad with zeros."""
        data = b"Now is the time for all "  # 24 bytes = 192 bits
        # With 64-bit blocks (8 bytes), already aligned, no padding
        padded = pad(data, 64, PaddingMethod.METHOD_1)
        assert len(padded) == 24
        assert padded == data

        # 23 bytes needs 1 byte padding
        data = b"Now is the time for all"
        padded = pad(data, 64, PaddingMethod.METHOD_1)
        assert len(padded) == 24
        assert padded == data + b"\x00"

    def test_padding_method_1_empty(self):
        """Test Padding Method 1 with empty data."""
        padded = pad(b"", 64, PaddingMethod.METHOD_1)
        assert len(padded) == 8  # n bits = 64 bits = 8 bytes
        assert padded == b"\x00" * 8

    def test_padding_method_2(self):
        """Test Padding Method 2: append 0x80 then pad with zeros."""
        data = b"Now is the time for all "  # 24 bytes
        padded = pad(data, 64, PaddingMethod.METHOD_2)
        # Already aligned, need full block of padding
        assert len(padded) == 32
        assert padded == data + b"\x80" + b"\x00" * 7

        data = b"Now is the time for all"  # 23 bytes
        padded = pad(data, 64, PaddingMethod.METHOD_2)
        assert len(padded) == 24
        assert padded == data + b"\x80"

    def test_padding_method_3(self):
        """Test Padding Method 3: prepend length block."""
        data = b"Now is the time for all "  # 24 bytes = 192 bits
        padded = pad(data, 64, PaddingMethod.METHOD_3)
        # Length block (8 bytes) + 24 bytes + padding to align
        assert len(padded) % 8 == 0
        # First 8 bytes should be length (192 = 0xC0)
        assert padded[:8] == b"\x00\x00\x00\x00\x00\x00\x00\xc0"

    def test_unpad(self):
        """Test unpadding reverses padding."""
        data = b"Hello"
        for method in [PaddingMethod.METHOD_1, PaddingMethod.METHOD_2]:
            padded = pad(data, 64, method)
            unpadded = unpad(padded, 64, method)
            assert unpadded == data


class TestKeyDerivation:
    """Test key derivation methods."""

    def test_multx_128(self):
        """Test multx operation for AES (n=128)."""
        # Test with zero input
        t = b"\x00" * 16
        result = multx(t, 128)
        assert result == b"\x00" * 16

        # Test with MSB=0: should just shift left
        t = b"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01"
        result = multx(t, 128)
        assert result == b"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x02"

        # Test with MSB=1: should shift and XOR with 0x87
        t = b"\x80" + b"\x00" * 15
        result = multx(t, 128)
        # (0x80 << 1) XOR 0x87 = 0x00 XOR 0x87 = 0x87... but in big-endian
        expected = b"\x00" * 15 + b"\x87"
        assert result == expected

    def test_multx_64(self):
        """Test multx operation for DES (n=64)."""
        t = b"\x00" * 8
        result = multx(t, 64)
        assert result == b"\x00" * 8

        # Test with MSB=1 for 64-bit
        t = b"\x80" + b"\x00" * 7
        result = multx(t, 64)
        expected = b"\x00" * 7 + b"\x1b"
        assert result == expected


class TestMACAlgorithm1:
    """Test MAC Algorithm 1 (CBC-MAC) with DES test vectors from Annex B."""

    def test_alg1_des_padding1_data1(self):
        """Test MAC Algorithm 1 with DES, Padding Method 1, Data string 1.

        From ISO/IEC 9797-1 Annex B.2:
        K = 0123456789ABCDEF
        Data = "Now is the time for all "
        Expected MAC = 70A30640 (32 bits)
        """
        key = bytes.fromhex("0123456789ABCDEF")
        data = b"Now is the time for all "

        mac = mac_algorithm_1(data, key, "DES", 32, PaddingMethod.METHOD_1)
        assert mac.hex().upper() == "70A30640"

    def test_alg1_des_padding2_data1(self):
        """Test MAC Algorithm 1 with DES, Padding Method 2, Data string 1.

        Expected MAC = 10E1F0F1 (32 bits)
        """
        key = bytes.fromhex("0123456789ABCDEF")
        data = b"Now is the time for all "

        mac = mac_algorithm_1(data, key, "DES", 32, PaddingMethod.METHOD_2)
        assert mac.hex().upper() == "10E1F0F1"

    def test_alg1_des_padding3_data1(self):
        """Test MAC Algorithm 1 with DES, Padding Method 3, Data string 1.

        Expected MAC = 2C58FB8F (32 bits)
        """
        key = bytes.fromhex("0123456789ABCDEF")
        data = b"Now is the time for all "

        mac = mac_algorithm_1(data, key, "DES", 32, PaddingMethod.METHOD_3)
        assert mac.hex().upper() == "2C58FB8F"

    def test_alg1_des_padding1_data2(self):
        """Test MAC Algorithm 1 with DES, Padding Method 1, Data string 2.

        Data = "Now is the time for it"
        Expected MAC = E45B3AD2 (32 bits)
        """
        key = bytes.fromhex("0123456789ABCDEF")
        data = b"Now is the time for it"

        mac = mac_algorithm_1(data, key, "DES", 32, PaddingMethod.METHOD_1)
        assert mac.hex().upper() == "E45B3AD2"

    def test_alg1_des_padding2_data2(self):
        """Test MAC Algorithm 1 with DES, Padding Method 2, Data string 2.

        From Annex B.2:
        Data = "Now is the time for it"
        Expected MAC = A924C721 (32 bits)
        """
        key = bytes.fromhex("0123456789ABCDEF")
        data = b"Now is the time for it"

        mac = mac_algorithm_1(data, key, "DES", 32, PaddingMethod.METHOD_2)
        assert mac.hex().upper() == "A924C721"

    def test_alg1_des_padding3_data2(self):
        """Test MAC Algorithm 1 with DES, Padding Method 3, Data string 2.

        From Annex B.2:
        Data = "Now is the time for it"
        Expected MAC = B1ECD6FC (32 bits)
        """
        key = bytes.fromhex("0123456789ABCDEF")
        data = b"Now is the time for it"

        mac = mac_algorithm_1(data, key, "DES", 32, PaddingMethod.METHOD_3)
        assert mac.hex().upper() == "B1ECD6FC"


class TestMACAlgorithm2:
    """Test MAC Algorithm 2 (EMAC) with DES test vectors."""

    def test_alg2_des_padding1_data1(self):
        """Test MAC Algorithm 2 with DES, Padding Method 1, Data string 1.

        From Annex B.3:
        K = 0123456789ABCDEF
        K' = F1D3B597795B3D1F (complement alternate 4-bit groups)
        Expected MAC = 10F9BC67 (32 bits)
        """
        key = bytes.fromhex("0123456789ABCDEF")
        key_prime = bytes.fromhex("F1D3B597795B3D1F")
        data = b"Now is the time for all "

        mac = mac_algorithm_2(data, key, key_prime, "DES", 32, PaddingMethod.METHOD_1)
        assert mac.hex().upper() == "10F9BC67"

    def test_alg2_des_padding2_data1(self):
        """Test MAC Algorithm 2 with DES, Padding Method 2, Data string 1.

        Expected MAC = BE7C2AB7 (32 bits)
        """
        key = bytes.fromhex("0123456789ABCDEF")
        key_prime = bytes.fromhex("F1D3B597795B3D1F")
        data = b"Now is the time for all "

        mac = mac_algorithm_2(data, key, key_prime, "DES", 32, PaddingMethod.METHOD_2)
        assert mac.hex().upper() == "BE7C2AB7"

    def test_alg2_des_padding3_data1(self):
        """Test MAC Algorithm 2 with DES, Padding Method 3, Data string 1.

        From Annex B.3:
        Expected MAC = 8EFC8BC7 (32 bits)
        """
        key = bytes.fromhex("0123456789ABCDEF")
        key_prime = bytes.fromhex("F1D3B597795B3D1F")
        data = b"Now is the time for all "

        mac = mac_algorithm_2(data, key, key_prime, "DES", 32, PaddingMethod.METHOD_3)
        assert mac.hex().upper() == "8EFC8BC7"

    def test_alg2_des_padding1_data2(self):
        """Test MAC Algorithm 2 with DES, Padding Method 1, Data string 2.

        From Annex B.3:
        Data = "Now is the time for it"
        Expected MAC = 215E9CE6 (32 bits)
        """
        key = bytes.fromhex("0123456789ABCDEF")
        key_prime = bytes.fromhex("F1D3B597795B3D1F")
        data = b"Now is the time for it"

        mac = mac_algorithm_2(data, key, key_prime, "DES", 32, PaddingMethod.METHOD_1)
        assert mac.hex().upper() == "215E9CE6"

    def test_alg2_des_padding2_data2(self):
        """Test MAC Algorithm 2 with DES, Padding Method 2, Data string 2.

        From Annex B.3:
        Expected MAC = 1736AC1A (32 bits)
        """
        key = bytes.fromhex("0123456789ABCDEF")
        key_prime = bytes.fromhex("F1D3B597795B3D1F")
        data = b"Now is the time for it"

        mac = mac_algorithm_2(data, key, key_prime, "DES", 32, PaddingMethod.METHOD_2)
        assert mac.hex().upper() == "1736AC1A"

    def test_alg2_des_padding3_data2(self):
        """Test MAC Algorithm 2 with DES, Padding Method 3, Data string 2.

        From Annex B.3:
        Expected MAC = 05382696 (32 bits)
        """
        key = bytes.fromhex("0123456789ABCDEF")
        key_prime = bytes.fromhex("F1D3B597795B3D1F")
        data = b"Now is the time for it"

        mac = mac_algorithm_2(data, key, key_prime, "DES", 32, PaddingMethod.METHOD_3)
        assert mac.hex().upper() == "05382696"


class TestMACAlgorithm3:
    """Test MAC Algorithm 3 (Retail MAC) with DES test vectors."""

    def test_alg3_des_padding1_data1(self):
        """Test MAC Algorithm 3 with DES, Padding Method 1, Data string 1.

        From Annex B.4:
        K = 0123456789ABCDEF
        K' = FEDCBA9876543210
        Expected MAC = A1C72E74 (32 bits)
        """
        key = bytes.fromhex("0123456789ABCDEF")
        key_prime = bytes.fromhex("FEDCBA9876543210")
        data = b"Now is the time for all "

        mac = mac_algorithm_3(data, key, key_prime, "DES", 32, PaddingMethod.METHOD_1)
        assert mac.hex().upper() == "A1C72E74"

    def test_alg3_des_padding2_data1(self):
        """Test MAC Algorithm 3 with DES, Padding Method 2, Data string 1.

        Expected MAC = E9086230 (32 bits)
        """
        key = bytes.fromhex("0123456789ABCDEF")
        key_prime = bytes.fromhex("FEDCBA9876543210")
        data = b"Now is the time for all "

        mac = mac_algorithm_3(data, key, key_prime, "DES", 32, PaddingMethod.METHOD_2)
        assert mac.hex().upper() == "E9086230"

    def test_alg3_des_padding3_data1(self):
        """Test MAC Algorithm 3 with DES, Padding Method 3, Data string 1.

        From Annex B.4:
        Expected MAC = AB059463 (32 bits)
        """
        key = bytes.fromhex("0123456789ABCDEF")
        key_prime = bytes.fromhex("FEDCBA9876543210")
        data = b"Now is the time for all "

        mac = mac_algorithm_3(data, key, key_prime, "DES", 32, PaddingMethod.METHOD_3)
        assert mac.hex().upper() == "AB059463"

    def test_alg3_des_padding1_data2(self):
        """Test MAC Algorithm 3 with DES, Padding Method 1, Data string 2.

        From Annex B.4:
        Data = "Now is the time for it"
        Expected MAC = 2E2B1428 (32 bits)
        """
        key = bytes.fromhex("0123456789ABCDEF")
        key_prime = bytes.fromhex("FEDCBA9876543210")
        data = b"Now is the time for it"

        mac = mac_algorithm_3(data, key, key_prime, "DES", 32, PaddingMethod.METHOD_1)
        assert mac.hex().upper() == "2E2B1428"

    def test_alg3_des_padding2_data2(self):
        """Test MAC Algorithm 3 with DES, Padding Method 2, Data string 2.

        From Annex B.4:
        Expected MAC = 5A692CE6 (32 bits)
        """
        key = bytes.fromhex("0123456789ABCDEF")
        key_prime = bytes.fromhex("FEDCBA9876543210")
        data = b"Now is the time for it"

        mac = mac_algorithm_3(data, key, key_prime, "DES", 32, PaddingMethod.METHOD_2)
        assert mac.hex().upper() == "5A692CE6"

    def test_alg3_des_padding3_data2(self):
        """Test MAC Algorithm 3 with DES, Padding Method 3, Data string 2.

        From Annex B.4:
        Expected MAC = C59F7EED (32 bits)
        """
        key = bytes.fromhex("0123456789ABCDEF")
        key_prime = bytes.fromhex("FEDCBA9876543210")
        data = b"Now is the time for it"

        mac = mac_algorithm_3(data, key, key_prime, "DES", 32, PaddingMethod.METHOD_3)
        assert mac.hex().upper() == "C59F7EED"


class TestMACAlgorithm5:
    """Test MAC Algorithm 5 (CMAC/OMAC1) with AES test vectors."""

    def test_alg5_aes128_empty(self):
        """Test MAC Algorithm 5 with AES-128, empty message.

        From Annex B.6.2:
        K = 2B7E151628AED2A6ABF7158809CF4F3C
        Expected G = BB1D6929E95937287FA37D129B756746
        """
        key = bytes.fromhex("2B7E151628AED2A6ABF7158809CF4F3C")
        data = b""

        mac = mac_algorithm_5(data, key, "AES", 128)
        assert mac.hex().upper() == "BB1D6929E95937287FA37D129B756746"

    def test_alg5_aes128_message(self):
        """Test MAC Algorithm 5 with AES-128, 16-byte message.

        From Annex B.6.2:
        K = 2B7E151628AED2A6ABF7158809CF4F3C
        Data = 6BC1BEE22E409F96E93D7E117393172A
        Expected G = 070A16B46B4D4144F79BDD9DD04A287C
        """
        key = bytes.fromhex("2B7E151628AED2A6ABF7158809CF4F3C")
        data = bytes.fromhex("6BC1BEE22E409F96E93D7E117393172A")

        mac = mac_algorithm_5(data, key, "AES", 128)
        assert mac.hex().upper() == "070A16B46B4D4144F79BDD9DD04A287C"

    def test_alg5_derive_keys(self):
        """Test key derivation for CMAC.

        From Annex B.6.2:
        S = e_K(0^128) = 7DF76B0C1AB899B33E42F047B91B546F
        K1 = FB EED6 18 35 71 33 66 7C 85 E0 8F 72 36 A8 DE
        K2 = F7 DD AC 30 6A E2 66 CC F9 0B C1 1E E4 6D 51 3B
        """
        key = bytes.fromhex("2B7E151628AED2A6ABF7158809CF4F3C")

        from Crypto.Cipher import AES

        cipher = AES.new(key, AES.MODE_ECB)
        k1, k2 = derive_keys_kdm2(key, 128, cipher.encrypt)

        assert k1.hex().upper() == "FB EED618357133667C85E08F7236A8DE".replace(" ", "")
        assert k2.hex().upper() == "F7 DDAC306AE266CCF90BC11EE46D513B".replace(" ", "")


class TestCalculateMAC:
    """Test the unified calculate_mac interface."""

    def test_calculate_mac_alg1(self):
        """Test calculate_mac with Algorithm 1."""
        key = bytes.fromhex("0123456789ABCDEF")
        data = b"Now is the time for all "

        mac = calculate_mac(
            data,
            key,
            MACAlgorithm.ALG_1,
            cipher_name="DES",
            mac_bits=32,
            padding_method=PaddingMethod.METHOD_1,
        )
        assert mac.hex().upper() == "70A30640"

    def test_calculate_mac_alg5(self):
        """Test calculate_mac with Algorithm 5."""
        key = bytes.fromhex("2B7E151628AED2A6ABF7158809CF4F3C")
        data = b""

        mac = calculate_mac(
            data,
            key,
            MACAlgorithm.ALG_5,
            cipher_name="AES",
            mac_bits=128,
        )
        assert mac.hex().upper() == "BB1D6929E95937287FA37D129B756746"

    def test_calculate_mac_defaults(self):
        """Test calculate_mac with default values."""
        key = bytes.fromhex("0123456789ABCDEF")
        data = b"test"

        mac = calculate_mac(data, key, MACAlgorithm.ALG_1, cipher_name="DES")
        # Should use default padding (METHOD_2) and full block size (64 bits)
        assert len(mac) == 8  # 64 bits = 8 bytes


class TestEdgeCases:
    """Test edge cases and error handling."""

    def test_empty_data(self):
        """Test MAC calculation with empty data."""
        key = bytes.fromhex("0123456789ABCDEF")

        # With padding method 2, empty data gets padded
        mac = mac_algorithm_1(b"", key, "DES", 32, PaddingMethod.METHOD_2)
        assert len(mac) == 4  # 32 bits = 4 bytes

    def test_full_block_data(self):
        """Test MAC calculation with exactly one block of data."""
        key = bytes.fromhex("0123456789ABCDEF")
        data = b"abcdefgh"  # Exactly 64 bits

        mac = mac_algorithm_1(data, key, "DES", 64, PaddingMethod.METHOD_1)
        assert len(mac) == 8

    def test_unknown_cipher(self):
        """Test error on unknown cipher."""
        with pytest.raises(ValueError, match="Unknown cipher"):
            calculate_mac(b"test", b"key", MACAlgorithm.ALG_1, cipher_name="UNKNOWN")


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
