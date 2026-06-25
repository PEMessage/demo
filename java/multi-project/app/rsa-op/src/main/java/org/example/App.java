package org.example;

import java.math.BigInteger;
import java.security.*;
import java.security.interfaces.RSAPrivateCrtKey;
import java.security.interfaces.RSAPrivateKey;
import java.security.interfaces.RSAPublicKey;
import java.security.spec.MGF1ParameterSpec;
import java.security.spec.PSSParameterSpec;

// org.bouncycastle.asn1.pkcs.RSAPrivateKey used fully qualified below
import org.bouncycastle.jce.provider.BouncyCastleProvider;

public class App {

    static {
        Security.addProvider(new BouncyCastleProvider());
    }

    private static final String SHA256_WITH_RSA = "SHA256withRSA";
    private static final String SHA256_WITH_RSA_PSS = "SHA256withRSA/PSS";
    private static final PSSParameterSpec PSS_PARAMS = new PSSParameterSpec(
            "SHA-256", "MGF1", MGF1ParameterSpec.SHA256, 32, 1);

    // ═══════════════════════════════════════════════════════════════
    // Main
    // ═══════════════════════════════════════════════════════════════

    public static void main(String[] args) throws Exception {
        System.out.println("=== BouncyCastle RSA Demo ===\n");

        RSAPrivateCrtKey privCrt;
        RSAPrivateKey priv;
        RSAPublicKey pub;
        // ── Key generation ─────────────────────────────────────────
        {
            KeyPair kp = genRsa(4096);
            privCrt = (RSAPrivateCrtKey) kp.getPrivate();
            priv = (RSAPrivateKey) kp.getPrivate();
            pub = (RSAPublicKey) kp.getPublic();
        }

        // ── Key info ──────────────────────────────────────────────
        {
            printRsaPrivateKey(privCrt);
            printKeyEncodings(privCrt, pub);
        }

        // ── Signing & Verification ────────────────────────────────
        {
            byte[] data = "12345678".getBytes();
            System.out.println("── [*] Signing & Verification");
            System.out.println("  Data : " + new String(data));

            byte[] sig = sign(data, priv);
            System.out.println("  PKCS#1 v1.5 sig (bytes) : " + sig.length);
            byte[] sigPss = signPss(data, priv);
            System.out.println("  PSS sig (bytes)         : " + sigPss.length);

            System.out.println("  Verify PKCS#1 v1.5      : " + verify(data, sig, pub));
            System.out.println("  Verify PSS              : " + verifyPss(data, sigPss, pub));
            System.out.println("  Cross verify (PSS sig with PKCS#1 v1.5) : "
                    + verify(data, sigPss, pub));
            System.out.println();
        }

        System.out.println("=== Done ===");
    }

    // ═══════════════════════════════════════════════════════════════
    // Key generation
    // ═══════════════════════════════════════════════════════════════

    static KeyPair genRsa(int keySize) throws Exception {
        System.out.println("── [*] Generating RSA " + keySize + " key pair...");
        KeyPairGenerator kpg = KeyPairGenerator.getInstance("RSA", "BC");
        kpg.initialize(keySize);
        KeyPair kp = kpg.generateKeyPair();
        System.out.println("Done.\n");
        return kp;
    }

    // ═══════════════════════════════════════════════════════════════
    // Print functions
    // ═══════════════════════════════════════════════════════════════

    static void printRsaPrivateKey(RSAPrivateCrtKey priv) {
        System.out.println("── [*] RSA Private Key Parameters (PKCS#1 RSAPrivateKey)");
        System.out.println("  modulus (n)         : " + fmt(priv.getModulus()));
        System.out.println("  publicExponent (e)  : " + priv.getPublicExponent());
        System.out.println("  privateExponent (d) : " + fmt(priv.getPrivateExponent()));
        System.out.println("  prime1 (p)          : " + fmt(priv.getPrimeP()));
        System.out.println("  prime2 (q)          : " + fmt(priv.getPrimeQ()));
        System.out.println("  exponent1 (dp)      : " + fmt(priv.getPrimeExponentP()));
        System.out.println("  exponent2 (dq)      : " + fmt(priv.getPrimeExponentQ()));
        System.out.println("  coefficient (qinv)  : " + fmt(priv.getCrtCoefficient()));
        System.out.println();
    }

    static void printKeyEncodings(RSAPrivateCrtKey priv, RSAPublicKey pub) throws Exception {
        System.out.println("── [*] Key Encodings");

        byte[] pkcs8 = priv.getEncoded();
        System.out.println("  Private key PKCS#8 (PrivateKeyInfo)  : " + pkcs8.length + " bytes");

        org.bouncycastle.asn1.pkcs.RSAPrivateKey pkcs1 =
                new org.bouncycastle.asn1.pkcs.RSAPrivateKey(
                priv.getModulus(), priv.getPublicExponent(), priv.getPrivateExponent(),
                priv.getPrimeP(), priv.getPrimeQ(),
                priv.getPrimeExponentP(), priv.getPrimeExponentQ(),
                priv.getCrtCoefficient());
        byte[] pkcs1Encoded = pkcs1.getEncoded();
        System.out.println("  Private key PKCS#1 (RSAPrivateKey)   : " + pkcs1Encoded.length + " bytes");

        byte[] pubEncoded = pub.getEncoded();
        System.out.println("  Public key  SubjectPublicKeyInfo     : " + pubEncoded.length + " bytes");
        System.out.println();
    }

    // ═══════════════════════════════════════════════════════════════
    // Sign & Verify
    // ═══════════════════════════════════════════════════════════════

    static byte[] sign(byte[] data, PrivateKey priv) throws Exception {
        Signature sig = Signature.getInstance(SHA256_WITH_RSA, "BC");
        sig.initSign(priv);
        sig.update(data);
        return sig.sign();
    }

    static byte[] signPss(byte[] data, PrivateKey priv) throws Exception {
        Signature sig = Signature.getInstance(SHA256_WITH_RSA_PSS, "BC");
        sig.setParameter(PSS_PARAMS);
        sig.initSign(priv);
        sig.update(data);
        return sig.sign();
    }

    static boolean verify(byte[] data, byte[] signature, PublicKey pub) throws Exception {
        Signature sig = Signature.getInstance(SHA256_WITH_RSA, "BC");
        sig.initVerify(pub);
        sig.update(data);
        return sig.verify(signature);
    }

    static boolean verifyPss(byte[] data, byte[] signature, PublicKey pub) throws Exception {
        Signature sig = Signature.getInstance(SHA256_WITH_RSA_PSS, "BC");
        sig.setParameter(PSS_PARAMS);
        sig.initVerify(pub);
        sig.update(data);
        return sig.verify(signature);
    }

    // ═══════════════════════════════════════════════════════════════
    // Utilities
    // ═══════════════════════════════════════════════════════════════

    static String hex(byte[] bytes) {
        StringBuilder sb = new StringBuilder();
        for (byte b : bytes) sb.append(String.format("%02x", b & 0xff));
        return sb.toString();
    }

    static String fmt(BigInteger bi) {
        String s = bi.toString(16);
        if (s.length() <= 72) return s;
        return s.substring(0, 36) + "..." + s.substring(s.length() - 36);
    }
}
