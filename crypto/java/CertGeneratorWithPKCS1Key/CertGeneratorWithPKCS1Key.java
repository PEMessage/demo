import java.io.FileInputStream;
import java.math.BigInteger;
import java.security.*;
import java.security.cert.X509Certificate;
import java.security.spec.RSAPublicKeySpec;
import java.security.spec.X509EncodedKeySpec;
import java.util.Date;
import javax.security.auth.x500.X500Principal;

import org.bouncycastle.asn1.ASN1Sequence;
import org.bouncycastle.asn1.pkcs.RSAPublicKey;
import org.bouncycastle.asn1.x509.SubjectPublicKeyInfo;
import org.bouncycastle.cert.X509v3CertificateBuilder;
import org.bouncycastle.cert.jcajce.JcaX509CertificateConverter;
import org.bouncycastle.operator.ContentSigner;
import org.bouncycastle.operator.jcajce.JcaContentSignerBuilder;

public class CertGeneratorWithPKCS1Key {

    public static void main(String[] args) throws Exception {
        if (args.length < 1) {
            System.err.println("Usage: java CertGeneratorWithPKCS1Key <pkcs1-public-key.der>");
            System.exit(1);
        }

        // Load BouncyCastle provider
        Security.addProvider(new org.bouncycastle.jce.provider.BouncyCastleProvider());

        // 1. Read the PKCS#1 DER format public key
        byte[] pkcs1Bytes = readFile(args[0]);
        
        // 2. Convert PKCS#1 to SubjectPublicKeyInfo (X.509 format)
        PublicKey subjectPublicKey = convertPkcs1ToPublicKey(pkcs1Bytes);
        
        // 3. Generate a random key pair for signing
        KeyPair signingKeyPair = generateKeyPair();
        
        // 4. Create a self-signed certificate
        X509Certificate certificate = generateCertificate(
                subjectPublicKey, 
                signingKeyPair.getPrivate(),
                "CN=Test Certificate");
        
        // 5. Output certificate information
        System.out.println("Generated Certificate:");
        System.out.println("Subject: " + certificate.getSubjectX500Principal());
        System.out.println("Issuer: " + certificate.getIssuerX500Principal());
        System.out.println("Serial: " + certificate.getSerialNumber());
        System.out.println("Valid from: " + certificate.getNotBefore());
        System.out.println("Valid until: " + certificate.getNotAfter());
        System.out.println("Public Key Algorithm: " + certificate.getPublicKey().getAlgorithm());
    }

    private static byte[] readFile(String filename) throws Exception {
        try (FileInputStream fis = new FileInputStream(filename)) {
            return fis.readAllBytes();
        }
    }

    private static PublicKey convertPkcs1ToPublicKey(byte[] pkcs1Bytes) throws Exception {
        // Parse PKCS#1 DER format
        ASN1Sequence sequence = ASN1Sequence.getInstance(pkcs1Bytes);
        RSAPublicKey rsaPubKey = RSAPublicKey.getInstance(sequence);
        
        // Create RSAPublicKeySpec from PKCS#1 parameters
        RSAPublicKeySpec pubKeySpec = new RSAPublicKeySpec(
                rsaPubKey.getModulus(),
                rsaPubKey.getPublicExponent());
        
        // Generate PublicKey object
        KeyFactory keyFactory = KeyFactory.getInstance("RSA");
        return keyFactory.generatePublic(pubKeySpec);
    }

    private static KeyPair generateKeyPair() throws Exception {
        KeyPairGenerator keyGen = KeyPairGenerator.getInstance("RSA");
        keyGen.initialize(2048); // Key size
        return keyGen.generateKeyPair();
    }

    private static X509Certificate generateCertificate(
            PublicKey subjectPublicKey,
            PrivateKey issuerPrivateKey,
            String subjectDN) throws Exception {
        
        // Certificate validity (1 year)
        Date startDate = new Date();
        Date endDate = new Date(startDate.getTime() + 365L * 24 * 60 * 60 * 1000);
        
        // Generate a random serial number
        BigInteger serial = BigInteger.valueOf(System.currentTimeMillis());
        
        // Certificate builder
        X509v3CertificateBuilder certBuilder = new X509v3CertificateBuilder(
                new X500Principal("CN=Temp CA"), // Issuer DN
                serial,
                startDate,
                endDate,
                new X500Principal(subjectDN), // Subject DN
                SubjectPublicKeyInfo.getInstance(subjectPublicKey.getEncoded()));
        
        // Sign the certificate
        ContentSigner signer = new JcaContentSignerBuilder("SHA256WithRSA")
                .build(issuerPrivateKey);
        
        // Convert to X509Certificate object
        return new JcaX509CertificateConverter()
                .setProvider("BC")
                .getCertificate(certBuilder.build(signer));
    }
}
