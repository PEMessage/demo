package org.example;

import java.security.Security;
import java.util.Arrays;
import java.util.Base64;
import javax.crypto.Cipher;
import javax.crypto.spec.IvParameterSpec;
import javax.crypto.spec.SecretKeySpec;
import org.bouncycastle.jce.provider.BouncyCastleProvider;

public class App {

    static {
        Security.addProvider(new BouncyCastleProvider());
    }

    /**
     * AES-CTR 加密
     * @param key  密钥，长度必须为 16/24/32 字节 (AES-128/192/256)
     * @param iv   初始向量，长度必须为 16 字节 (AES 块大小)
     * @param data 明文数据，任意长度，CTR 模式无需填充
     * @return 密文，长度与明文一致
     */
    public static byte[] aesCtrEncrypt(byte[] key, byte[] iv, byte[] data) throws Exception {
        return aesCtr(Cipher.ENCRYPT_MODE, key, iv, data);
    }

    /**
     * AES-CTR 解密
     * @param key  密钥，长度必须为 16/24/32 字节 (AES-128/192/256)
     * @param iv   初始向量，长度必须为 16 字节 (AES 块大小)
     * @param data 密文数据
     * @return 明文，长度与密文一致
     */
    public static byte[] aesCtrDecrypt(byte[] key, byte[] iv, byte[] data) throws Exception {
        return aesCtr(Cipher.DECRYPT_MODE, key, iv, data);
    }

    /**
     * CTR 模式下 IV 长度必须等于 AES 块大小 16 字节，JCE 默认不校验，但与加密方不一致会导致解密错误。
     * key 长度限制为 AES 标准的三种: 16 (AES-128), 24 (AES-192), 32 (AES-256)。
     */
    private static byte[] aesCtr(int mode, byte[] key, byte[] iv, byte[] data) throws Exception {
        if (key.length != 16 && key.length != 24 && key.length != 32) {
            throw new IllegalArgumentException("Key must be 16, 24, or 32 bytes long");
        }
        if (iv.length != 16) {
            throw new IllegalArgumentException("IV must be 16 bytes long (AES block size)");
        }

        Cipher cipher = Cipher.getInstance("AES/CTR/NoPadding", "BC");
        cipher.init(mode, new SecretKeySpec(key, "AES"), new IvParameterSpec(iv));
        return cipher.doFinal(data);
    }

    public static void main(String[] args) throws Exception {
        byte[] key = new byte[16];
        byte[] iv = new byte[16];
        byte[] plaintext = "Hello, AES-CTR!".getBytes();
        Arrays.fill(key, (byte) 0x11);
        Arrays.fill(iv, (byte) 0x22);

        System.out.println("Key:     " + bytesToHex(key));
        System.out.println("IV:      " + bytesToHex(iv));
        System.out.println("Plain:   " + new String(plaintext));

        byte[] ciphertext = aesCtrEncrypt(key, iv, plaintext);
        System.out.println("Encrypt: " + bytesToHex(ciphertext));

        byte[] decrypted = aesCtrDecrypt(key, iv, ciphertext);
        System.out.println("Decrypt: " + new String(decrypted));

        System.out.println("Match:   " + Arrays.equals(plaintext, decrypted));
    }

    private static String bytesToHex(byte[] bytes) {
        StringBuilder sb = new StringBuilder();
        for (byte b : bytes) {
            sb.append(String.format("%02x", b));
        }
        return sb.toString();
    }
}
