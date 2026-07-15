package org.example;

import org.junit.jupiter.api.Test;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;

import static org.junit.jupiter.api.Assertions.*;

public class AppTest {

    @Test
    public void testEncryptDecrypt() throws Exception {
        byte[] key = new byte[16];
        byte[] iv = new byte[16];
        Arrays.fill(key, (byte) 0x33);
        Arrays.fill(iv, (byte) 0x44);

        String original = "Hello AES-CTR!";
        byte[] plaintext = original.getBytes(StandardCharsets.UTF_8);
        byte[] ciphertext = App.aesCtrEncrypt(key, iv, plaintext);
        byte[] decrypted = App.aesCtrDecrypt(key, iv, ciphertext);

        assertEquals(plaintext.length, ciphertext.length);
        assertEquals(original, new String(decrypted, StandardCharsets.UTF_8));
    }

    @Test
    public void testEncryptDecrypt32ByteKey() throws Exception {
        byte[] key = new byte[32];
        byte[] iv = new byte[16];
        Arrays.fill(key, (byte) 0x55);

        byte[] plaintext = "Test with 256-bit key".getBytes(StandardCharsets.UTF_8);
        byte[] ciphertext = App.aesCtrEncrypt(key, iv, plaintext);
        byte[] decrypted = App.aesCtrDecrypt(key, iv, ciphertext);

        assertArrayEquals(plaintext, decrypted);
    }

    @Test
    public void testWrongKey() throws Exception {
        byte[] key1 = new byte[16];
        byte[] key2 = new byte[16];
        byte[] iv = new byte[16];
        Arrays.fill(key1, (byte) 0x11);
        Arrays.fill(key2, (byte) 0x22);

        byte[] plaintext = "secret".getBytes(StandardCharsets.UTF_8);
        byte[] ciphertext = App.aesCtrEncrypt(key1, iv, plaintext);
        byte[] decrypted = App.aesCtrDecrypt(key2, iv, ciphertext);

        assertFalse(Arrays.equals(plaintext, decrypted));
    }
}
