package org.example;

import javax.crypto.Cipher;
import javax.crypto.SecretKey;
import javax.crypto.spec.IvParameterSpec;
import javax.crypto.spec.SecretKeySpec;
import java.security.Security;
import java.util.Arrays;
import org.bouncycastle.jce.provider.BouncyCastleProvider;

public class App {
    // Constants for modes
    public static final int ECB_MODE = 0;
    public static final int CBC_MODE = 1;
    public static final int ENCRYPT_MODE = 0;
    public static final int DECRYPT_MODE = 1;

    public static void main(String[] args) {
        try {
            // Add BouncyCastle provider if not already added
            Security.addProvider(new BouncyCastleProvider());
            // Create key and data filled with 0x11 (32 bytes each)
            byte[] key = new byte[16];
            byte[] data = new byte[16];
            Arrays.fill(key, (byte)0x11);
            Arrays.fill(data, (byte)0x11);
            data[15] = 0x10;


            byte[] ciphertext = aesCalc(ENCRYPT_MODE, ECB_MODE, key, data);
            System.out.println("Ciphertext (hex): " + bytesToHex(ciphertext));


        } catch (Exception e) {
            e.printStackTrace();
        }
    }
    // Helper method to convert byte array to hex string
    private static String bytesToHex(byte[] bytes) {
        StringBuilder sb = new StringBuilder();
        for (byte b : bytes) {
            sb.append(String.format("%02x", b));
        }
        return sb.toString();
    }

    public static byte[] aesCalc(int calMode, int blkMode, byte[] key, byte[] data) throws Exception {
        return aesCalc(calMode, blkMode, key, data, null);
    }

    public static byte[] aesCalc(int calMode, int blkMode, byte[] key, byte[] data, byte[] initial_vector) throws Exception {

        String algFormat;
        String blockMode;
        int opMode;
        IvParameterSpec iv = null;

        if (key.length != 16 && key.length != 24 && key.length != 32) {
            throw new IllegalArgumentException("Key must be 16, 24, or 32 bytes long");
        }

        if (blkMode == ECB_MODE) {
            blockMode = "ECB";
        } else if (blkMode == CBC_MODE) {
            blockMode = "CBC";
            if (initial_vector != null) {
                iv = new IvParameterSpec(initial_vector);
            }
            else {
                iv = new IvParameterSpec(new byte[16]);
            }
        } else {
            throw new IllegalArgumentException("Invalid block mode");
        }

        if (calMode == ENCRYPT_MODE) {
            opMode = Cipher.ENCRYPT_MODE;
        } else if (calMode == DECRYPT_MODE) {
            opMode = Cipher.DECRYPT_MODE;
        } else {
            throw new IllegalArgumentException("Invalid calculation mode");
        }

        algFormat = "AES/" + blockMode + "/NoPadding";
        Cipher cipher = Cipher.getInstance(algFormat, "BC");

        SecretKey secretKey = new SecretKeySpec(key, "AES");

        if (blkMode == CBC_MODE) {
            cipher.init(opMode, secretKey, iv);
        } else {
            cipher.init(opMode, secretKey);
        }

        if (data.length % 16 != 0) {
            byte[] aligned = new byte[(data.length + 15) / 16 * 16];
            System.arraycopy(data, 0, aligned, 0, data.length);
            return cipher.doFinal(aligned);    
        }

        return cipher.doFinal(data);
    }
}
