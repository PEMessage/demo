package org.example;

import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;

import java.nio.charset.StandardCharsets;

/*
 * Test vectors from:
 *   NIST SP 800-38B (May 2005), Appendix D "Examples"
 *   https://nvlpubs.nist.gov/nistpubs/Legacy/SP/nistspecialpublication800-38b.pdf
 *   with corrections from the errata appended at the end of the document.
 *
 *   RFC 4493 Section 4 (same AES-128 vectors)
 *   https://datatracker.ietf.org/doc/html/rfc4493
 *
 * All 20 examples:
 *   D.1  AES-128          Examples 1-4
 *   D.2  AES-192          Examples 5-8
 *   D.3  AES-256          Examples 9-12
 *   D.4  Three Key TDEA   Examples 13-16 (errata: 14,15 corrected)
 *   D.5  Two Key TDEA     Examples 17-20 (errata: 18,19 corrected)
 */

public class AppTest {

    // ---- D.1 AES-128 (same as RFC 4493 Section 4) ----

    /*
     * NIST SP 800-38B Appendix D.1 / RFC 4493 Section 4
     * K  = 2b7e1516 28aed2a6 abf71588 09cf4f3c
     * CIPH_K(0^128) = 7df76b0c 1ab899b3 3e42f047 b91b546f
     * K1 = fbeed618 35713366 7c85e08f 7236a8de
     * K2 = f7ddac30 6ae266cc f90bc11e e46d513b
     */
    @Test public void testAes128Cmac_Example1_Mlen0() {
        byte[] key = h("2b7e151628aed2a6abf7158809cf4f3c");
        assertArrayEquals(h("bb1d6929e95937287fa37d129b756746"), App.aesCmac(key, new byte[0]));
    }

    @Test public void testAes128Cmac_Example2_Mlen128() {
        byte[] key = h("2b7e151628aed2a6abf7158809cf4f3c");
        byte[] msg = h("6bc1bee22e409f96e93d7e117393172a");
        assertArrayEquals(h("070a16b46b4d4144f79bdd9dd04a287c"), App.aesCmac(key, msg));
    }

    @Test public void testAes128Cmac_Example3_Mlen320() {
        byte[] key = h("2b7e151628aed2a6abf7158809cf4f3c");
        byte[] msg = h("6bc1bee22e409f96e93d7e117393172a"
                + "ae2d8a571e03ac9c9eb76fac45af8e51"
                + "30c81c46a35ce411");
        assertArrayEquals(h("dfa66747de9ae63030ca32611497c827"), App.aesCmac(key, msg));
    }

    @Test public void testAes128Cmac_Example4_Mlen512() {
        byte[] key = h("2b7e151628aed2a6abf7158809cf4f3c");
        byte[] msg = h("6bc1bee22e409f96e93d7e117393172a"
                + "ae2d8a571e03ac9c9eb76fac45af8e51"
                + "30c81c46a35ce411e5fbc1191a0a52ef"
                + "f69f2445df4f9b17ad2b417be66c3710");
        assertArrayEquals(h("51f0bebf7e3b9d92fc49741779363cfe"), App.aesCmac(key, msg));
    }

    // ---- D.2 AES-192 ----

    /*
     * NIST SP 800-38B Appendix D.2
     * K  = 8e73b0f7 da0e6452 c810f32b 809079e5 62f8ead2 522c6b7b
     * CIPH_K(0^128) = 22452d8e 49a8a593 9f7321ce ea6d514b
     * K1 = 448a5b1c 93514b27 3ee6439d d4daa296
     * K2 = 8914b639 26a2964e 7dcc873b a9b5452c
     */
    @Test public void testAes192Cmac_Example5_Mlen0() {
        byte[] key = h("8e73b0f7da0e6452c810f32b809079e562f8ead2522c6b7b");
        assertArrayEquals(h("d17ddf46adaacde531cac483de7a9367"), App.aesCmac(key, new byte[0]));
    }

    @Test public void testAes192Cmac_Example6_Mlen128() {
        byte[] key = h("8e73b0f7da0e6452c810f32b809079e562f8ead2522c6b7b");
        byte[] msg = h("6bc1bee22e409f96e93d7e117393172a");
        assertArrayEquals(h("9e99a7bf31e710900662f65e617c5184"), App.aesCmac(key, msg));
    }

    @Test public void testAes192Cmac_Example7_Mlen320() {
        byte[] key = h("8e73b0f7da0e6452c810f32b809079e562f8ead2522c6b7b");
        byte[] msg = h("6bc1bee22e409f96e93d7e117393172a"
                + "ae2d8a571e03ac9c9eb76fac45af8e51"
                + "30c81c46a35ce411");
        assertArrayEquals(h("8a1de5be2eb31aad089a82e6ee908b0e"), App.aesCmac(key, msg));
    }

    @Test public void testAes192Cmac_Example8_Mlen512() {
        byte[] key = h("8e73b0f7da0e6452c810f32b809079e562f8ead2522c6b7b");
        byte[] msg = h("6bc1bee22e409f96e93d7e117393172a"
                + "ae2d8a571e03ac9c9eb76fac45af8e51"
                + "30c81c46a35ce411e5fbc1191a0a52ef"
                + "f69f2445df4f9b17ad2b417be66c3710");
        assertArrayEquals(h("a1d5df0eed790f794d77589659f39a11"), App.aesCmac(key, msg));
    }

    // ---- D.3 AES-256 ----

    /*
     * NIST SP 800-38B Appendix D.3
     * K  = 603deb10 15ca71be 2b73aef0 857d7781 1f352c07 3b6108d7 2d9810a3 0914dff4
     * CIPH_K(0^128) = e568f681 94cf76d6 174d4cc0 4310a854
     * K1 = cad1ed03 299eedac 2e9a9980 8621502f
     * K2 = 95a3da06 533ddb58 5d353301 0c42a0d9
     */
    @Test public void testAes256Cmac_Example9_Mlen0() {
        byte[] key = h("603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4");
        assertArrayEquals(h("028962f61b7bf89efc6b551f4667d983"), App.aesCmac(key, new byte[0]));
    }

    @Test public void testAes256Cmac_Example10_Mlen128() {
        byte[] key = h("603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4");
        byte[] msg = h("6bc1bee22e409f96e93d7e117393172a");
        assertArrayEquals(h("28a7023f452e8f82bd4bf28d8c37c35c"), App.aesCmac(key, msg));
    }

    @Test public void testAes256Cmac_Example11_Mlen320() {
        byte[] key = h("603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4");
        byte[] msg = h("6bc1bee22e409f96e93d7e117393172a"
                + "ae2d8a571e03ac9c9eb76fac45af8e51"
                + "30c81c46a35ce411");
        assertArrayEquals(h("aaf3d8f1de5640c232f5b169b9c911e6"), App.aesCmac(key, msg));
    }

    @Test public void testAes256Cmac_Example12_Mlen512() {
        byte[] key = h("603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4");
        byte[] msg = h("6bc1bee22e409f96e93d7e117393172a"
                + "ae2d8a571e03ac9c9eb76fac45af8e51"
                + "30c81c46a35ce411e5fbc1191a0a52ef"
                + "f69f2445df4f9b17ad2b417be66c3710");
        assertArrayEquals(h("e1992190549f6ed5696a2c056c315410"), App.aesCmac(key, msg));
    }

    // ---- D.4 Three Key TDEA ----

    /*
     * NIST SP 800-38B Appendix D.4 (errata: Examples 14, 15 corrected)
     * Key1 = 8aa83bf8 cbda1062
     * Key2 = 0bc1bf19 fbb6cd58
     * Key3 = bc313d4a 371ca8b5
     * K     = Key1 || Key2 || Key3
     * CIPH_K(0^64)  = c8cc74e9 8a7329a2
     * K1    = 9198e9d3 14e6535f
     * K2    = 2331d3a6 29cca6a5
     */
    private static final byte[] TDES3_KEY = h("8aa83bf8cbda10620bc1bf19fbb6cd58bc313d4a371ca8b5");

    @Test public void testTdes3Cmac_Example13_Mlen0() {
        assertArrayEquals(h("b7a688e122ffaf95"), App.desCmac(TDES3_KEY, new byte[0]));
    }

    // Errata corrected: original was "b7a688e1 22ffaf95"
    @Test public void testTdes3Cmac_Example14_Mlen64() {
        byte[] msg = h("6bc1bee22e409f96"); // 8 bytes (= 64 bits)
        assertArrayEquals(h("8e8f293136283797"), App.desCmac(TDES3_KEY, msg));
    }

    // Errata corrected: original was "d32bcebe 43d23d80"
    @Test public void testTdes3Cmac_Example15_Mlen160() {
        byte[] msg = h("6bc1bee22e409f96e93d7e117393172aae2d8a57"); // 20 bytes (= 160 bits)
        assertArrayEquals(h("743ddbe0ce2dc2ed"), App.desCmac(TDES3_KEY, msg));
    }

    @Test public void testTdes3Cmac_Example16_Mlen256() {
        byte[] msg = h("6bc1bee22e409f96e93d7e117393172aae2d8a571e03ac9c9eb76fac45af8e51"); // 32 bytes (= 256 bits)
        assertArrayEquals(h("33e6b1092400eae5"), App.desCmac(TDES3_KEY, msg));
    }

    // ---- D.5 Two Key TDEA ----

    /*
     * NIST SP 800-38B Appendix D.5 (errata: Examples 18, 19 corrected)
     * Key1 = 4cf15134 a2850dd5
     * Key2 = 8a3d10ba 80570d38
     * Key3 = Key1
     * K     = Key1 || Key2 || Key1
     * CIPH_K(0^64)  = c7679b9f 6b8d7d7a
     * K1    = 8ecf373e d71afaef
     * K2    = 1d9e6e7d ae35f5c5
     */
    private static final byte[] TDES2_KEY = h("4cf15134a2850dd58a3d10ba80570d384cf15134a2850dd5");

    @Test public void testTdes2Cmac_Example17_Mlen0() {
        assertArrayEquals(h("bd2ebf9a3ba00361"), App.desCmac(TDES2_KEY, new byte[0]));
    }

    // Errata corrected: original was "bd2ebf9a 3ba00361"
    @Test public void testTdes2Cmac_Example18_Mlen64() {
        byte[] msg = h("6bc1bee22e409f96"); // 8 bytes (= 64 bits)
        assertArrayEquals(h("4ff2ab813c53ce83"), App.desCmac(TDES2_KEY, msg));
    }

    // Errata corrected: original was "8ea92435 b52660e0"
    @Test public void testTdes2Cmac_Example19_Mlen160() {
        byte[] msg = h("6bc1bee22e409f96e93d7e117393172aae2d8a57"); // 20 bytes (= 160 bits)
        assertArrayEquals(h("62dd1b471902bd4e"), App.desCmac(TDES2_KEY, msg));
    }

    @Test public void testTdes2Cmac_Example20_Mlen256() {
        byte[] msg = h("6bc1bee22e409f96e93d7e117393172aae2d8a571e03ac9c9eb76fac45af8e51"); // 32 bytes (= 256 bits)
        assertArrayEquals(h("31b1e431dabc4eb8"), App.desCmac(TDES2_KEY, msg));
    }

    // ---- auxiliary tests (truncation, determinism, etc.) ----

    private static final byte[] AES128_KEY = h("2b7e151628aed2a6abf7158809cf4f3c");

    @Test public void testAesCmacTruncate() {
        byte[] msg = "hello".getBytes(StandardCharsets.UTF_8);
        byte[] full = App.aesCmac(AES128_KEY, msg);
        byte[] truncated = App.aesCmac(AES128_KEY, msg, 8);
        assertEquals(8, truncated.length);
        for (int i = 0; i < 8; i++) {
            assertEquals(full[i], truncated[i]);
        }
    }

    @Test public void testTruncateInvalid() {
        byte[] msg = "test".getBytes(StandardCharsets.UTF_8);
        assertThrows(IllegalArgumentException.class, () -> App.aesCmac(AES128_KEY, msg, 0));
        assertThrows(IllegalArgumentException.class, () -> App.aesCmac(AES128_KEY, msg, 17));
    }

    @Test public void testCmacDeterministic() {
        byte[] msg = "data".getBytes(StandardCharsets.UTF_8);
        byte[] mac1 = App.aesCmac(AES128_KEY, msg);
        byte[] mac2 = App.aesCmac(AES128_KEY, msg);
        assertArrayEquals(mac1, mac2);
    }

    @Test public void testDifferentMsgDifferentMac() {
        byte[] msg1 = "apple".getBytes(StandardCharsets.UTF_8);
        byte[] msg2 = "orange".getBytes(StandardCharsets.UTF_8);
        assertFalse(java.util.Arrays.equals(App.aesCmac(AES128_KEY, msg1), App.aesCmac(AES128_KEY, msg2)));
    }

    @Test public void testCbcMacBasic() throws Exception {
        byte[] mac = App.aesCbcMac(AES128_KEY, "Hello, World!".getBytes(StandardCharsets.UTF_8));
        assertEquals(16, mac.length);
    }

    @Test public void testCbcMacDeterministic() throws Exception {
        byte[] msg = "Hello, World!".getBytes(StandardCharsets.UTF_8);
        byte[] mac1 = App.aesCbcMac(AES128_KEY, msg);
        byte[] mac2 = App.aesCbcMac(AES128_KEY, msg);
        assertArrayEquals(mac1, mac2);
    }

    /*
     * FIPS PUB 113, Appendix 2 "An Example of the DAA"
     * https://nvlpubs.nist.gov/nistpubs/Legacy/FIPS/fipspub113.pdf
     *
     * This CBC-MAC (zero-padding, zero-IV) is also ISO/IEC 9797-1 MAC Algorithm 1:
     * https://en.wikipedia.org/wiki/ISO/IEC_9797-1#MAC_algorithm_1
     *
     * Also referenced by rust-crypto CBC-MAC implementation:
     * https://github.com/RustCrypto/MACs/tree/master/cbc-mac
     *
     * NOTE: The spec hex has a known typo — 0x68 should be 0x69 (byte 12: 'h'→'i').
     * Fixed hex below yields the correct DES CBC-MAC.
     *
     * Key   = 0123456789abcdef (8-byte DES)
     * Text  = "7654321 Now is the time for "
     * DAC   = f1d30f68 (32-bit truncation)
     * Full 8-byte MAC = f1d30f6849312ca4
     */
    @Test public void testDesCbcMac_Fips113() throws Exception {
        byte[] key = h("0123456789abcdef");
        byte[] msg = h("37363534333231204e6f77206973207468652074696d6520666f7220");
        assertArrayEquals(h("f1d30f6849312ca4"), App.desCbcMac(key, msg));
    }

    @Test public void testCbcMacVsCmacDifferent() throws Exception {
        byte[] msg = "test".getBytes(StandardCharsets.UTF_8);
        byte[] cbc = App.aesCbcMac(AES128_KEY, msg);
        byte[] cmac = App.aesCmac(AES128_KEY, msg);
        assertFalse(java.util.Arrays.equals(cbc, cmac));
    }

    // ---- helper ----

    private static byte[] h(String hex) {
        int len = hex.length();
        byte[] data = new byte[len / 2];
        for (int i = 0; i < len; i += 2) {
            data[i / 2] = (byte) ((Character.digit(hex.charAt(i), 16) << 4)
                    | Character.digit(hex.charAt(i + 1), 16));
        }
        return data;
    }
}
