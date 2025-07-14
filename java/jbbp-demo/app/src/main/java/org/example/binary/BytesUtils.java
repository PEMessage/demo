package org.example.binary;


public class BytesUtils {

    /**
     * Converts a hexadecimal string to a byte array
     * @param s the string to be converted, length must be even
     * @return the converted byte array, length is half of the string length
     */
    public static byte[] hexToByteArray(String s) {
        int len = s.length();
        byte[] data = new byte[len / 2];
        for (int i = 0; i < len; i += 2) {
            data[i / 2] = (byte) ((Character.digit(s.charAt(i), 16) << 4)
                                  + Character.digit(s.charAt(i + 1), 16));
        }
        return data;
    }

    /**
     * Converts binary data to a hexadecimal string
     *
     * @param data the binary data to be converted
     * @return the converted hexadecimal string
     */
    public static String bytes2HexString(byte[] data) {
        if (data == null) {
            return "";
        }
        StringBuilder buffer = new StringBuilder();
        for (byte b : data) {
            String hex = Integer.toHexString(b & 0xff);
            if (hex.length() == 1) {
                buffer.append('0');
            }
            buffer.append(hex);
        }
        return buffer.toString().toUpperCase();
    }

    // Thanks to kimi-k2
    // prompt: cat two byte[] in java
    public static byte[] concat(byte[] a, byte[] b) {
        byte[] result = new byte[a.length + b.length];
        System.arraycopy(a, 0, result, 0, a.length);
        System.arraycopy(b, 0, result, a.length, b.length);
        return result;
    }
}
