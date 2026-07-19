package org.example;

import java.security.Security;
import java.util.Arrays;
import javax.crypto.Cipher;
import javax.crypto.spec.IvParameterSpec;
import javax.crypto.spec.SecretKeySpec;

import org.bouncycastle.crypto.BlockCipher;
import org.bouncycastle.crypto.engines.AESEngine;
import org.bouncycastle.crypto.engines.DESEngine;
import org.bouncycastle.crypto.engines.DESedeEngine;
import org.bouncycastle.crypto.macs.CMac;
import org.bouncycastle.crypto.params.KeyParameter;
import org.bouncycastle.jce.provider.BouncyCastleProvider;

public class App {

    static {
        Security.addProvider(new BouncyCastleProvider());
    }

    private static final int AES_BLOCK = 16;
    private static final int DES_BLOCK = 8;

    // ---- CBC-MAC (zero-padding, zero-IV) ----

    public static byte[] aesCbcMac(byte[] key, byte[] msg) throws Exception {
        return cbcMac("AES", AES_BLOCK, key, msg);
    }

    public static byte[] desCbcMac(byte[] key, byte[] msg) throws Exception {
        String algo = key.length == 8 ? "DES" : "DESede";
        return cbcMac(algo, DES_BLOCK, key, msg);
    }

    private static byte[] cbcMac(String algorithm, int blockSize, byte[] key, byte[] msg) throws Exception {
        byte[] padded = zeroPad(msg, blockSize);
        Cipher cipher = Cipher.getInstance(algorithm + "/CBC/NoPadding", "BC");
        cipher.init(Cipher.ENCRYPT_MODE,
                new SecretKeySpec(key, algorithm),
                new IvParameterSpec(new byte[blockSize]));
        byte[] encrypted = cipher.doFinal(padded);
        return Arrays.copyOfRange(encrypted, encrypted.length - blockSize, encrypted.length);
    }

    // ---- CMAC (BouncyCastle light API) ----

    public static byte[] aesCmac(byte[] key, byte[] msg) {
        return cmac(new AESEngine(), key, msg, AES_BLOCK);
    }

    public static byte[] aesCmac(byte[] key, byte[] msg, int len) {
        return truncate(aesCmac(key, msg), len);
    }

    public static byte[] desCmac(byte[] key, byte[] msg) {
        BlockCipher engine = key.length == 8 ? new DESEngine() : new DESedeEngine();
        return cmac(engine, key, msg, DES_BLOCK);
    }

    public static byte[] desCmac(byte[] key, byte[] msg, int len) {
        return truncate(desCmac(key, msg), len);
    }

    private static byte[] cmac(BlockCipher engine, byte[] key, byte[] msg, int expectedSize) {
        CMac cmac = new CMac(engine);
        cmac.init(new KeyParameter(key));
        cmac.update(msg, 0, msg.length);
        byte[] out = new byte[cmac.getMacSize()];
        cmac.doFinal(out, 0);
        if (out.length != expectedSize) {
            throw new IllegalStateException(
                    "Unexpected MAC size: " + out.length + ", expected " + expectedSize);
        }
        return out;
    }

    // ---- utilities ----

    private static byte[] truncate(byte[] mac, int len) {
        if (len <= 0 || len > mac.length) {
            throw new IllegalArgumentException("truncation len " + len + " must be in [1, " + mac.length + "]");
        }
        return Arrays.copyOf(mac, len);
    }

    private static byte[] zeroPad(byte[] data, int blockSize) {
        int padLen = blockSize - (data.length % blockSize);
        if (padLen == blockSize) {
            padLen = 0;
        }
        byte[] padded = new byte[data.length + padLen];
        System.arraycopy(data, 0, padded, 0, data.length);
        return padded;
    }

    private static String bytesToHex(byte[] bytes) {
        StringBuilder sb = new StringBuilder();
        for (byte b : bytes) {
            sb.append(String.format("%02x", b));
        }
        return sb.toString();
    }

    // ---- demo ----

    public static void main(String[] args) throws Exception {
        byte[] aesKey = hexToBytes("2b7e151628aed2a6abf7158809cf4f3c");
        byte[] desKey = hexToBytes("0123456789abcdef");
        byte[] tdesKey = hexToBytes("0123456789abcdeffedcba98765432100123456789abcdef");
        byte[] msg = "Hello, MAC!".getBytes();

        System.out.println("message: " + new String(msg));

        byte[] aesCbc = aesCbcMac(aesKey, msg);
        System.out.println("AES-CBC-MAC : " + bytesToHex(aesCbc));

        byte[] aesCmac = aesCmac(aesKey, msg);
        System.out.println("AES-CMAC    : " + bytesToHex(aesCmac));

        byte[] desCbc = desCbcMac(desKey, msg);
        System.out.println("DES-CBC-MAC : " + bytesToHex(desCbc));

        byte[] desCmac = desCmac(desKey, msg);
        System.out.println("DES-CMAC    : " + bytesToHex(desCmac));

        byte[] tdesCbc = desCbcMac(tdesKey, msg);
        System.out.println("3DES-CBC-MAC: " + bytesToHex(tdesCbc));

        byte[] tdesCmac = desCmac(tdesKey, msg);
        System.out.println("3DES-CMAC   : " + bytesToHex(tdesCmac));

        System.out.println("trim AES-CMAC[8]: " + bytesToHex(aesCmac(aesKey, msg, 8)));
    }

    private static byte[] hexToBytes(String hex) {
        int len = hex.length();
        byte[] data = new byte[len / 2];
        for (int i = 0; i < len; i += 2) {
            data[i / 2] = (byte) ((Character.digit(hex.charAt(i), 16) << 4)
                    | Character.digit(hex.charAt(i + 1), 16));
        }
        return data;
    }
}
