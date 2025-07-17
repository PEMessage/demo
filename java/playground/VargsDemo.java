
public class VargsDemo {

    public static void main(String[] args) {
        // Example 1: Using byte...
        processBytes((byte)1, (byte)2, (byte)3, (byte)4, (byte)5); // varargs
        processBytes(); // varargs
        processBytes(new byte[] {}); // varargs
        processBytes(new byte[]{1, 2, 3, 4, 5}); // also works with array

        // Example 2: Using byte[]
        // processBytes((byte)1, (byte)2, (byte)3, (byte)4, (byte)5); // not compile
        // processBytesArray(); // not compile !
        processBytes(new byte[] {}); // varargs
        processBytesArray(new byte[]{1, 2, 3, 4, 5}); // also works with array
    }

    // Method using byte varargs (byte...)
    public static void processBytes(byte... bytes) {
        System.out.println("\nProcessing bytes (varargs):");
        System.out.println("Number of bytes: " + bytes.length);
        for (byte b : bytes) {
            System.out.print(b + " ");
        }
        System.out.println();
    }

    // Method using byte array (byte[])
    public static void processBytesArray(byte[] bytes) {
        System.out.println("\nProcessing byte array:");
        System.out.println("Array length: " + bytes.length);
        for (byte b : bytes) {
            System.out.print(b + " ");
        }
        System.out.println();
    }
}
