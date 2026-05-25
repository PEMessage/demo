package org.example;

import org.bouncycastle.asn1.x500.X500Name;
import org.bouncycastle.cert.X509v3CertificateBuilder;
import org.bouncycastle.cert.jcajce.JcaX509CertificateConverter;
import org.bouncycastle.cert.jcajce.JcaX509v3CertificateBuilder;
import org.bouncycastle.jce.provider.BouncyCastleProvider;
import org.bouncycastle.operator.ContentSigner;
import org.bouncycastle.operator.jcajce.JcaContentSignerBuilder;

import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.math.BigInteger;
import java.security.KeyFactory;
import java.security.PublicKey;
import java.security.SecureRandom;
import java.security.Security;
import java.security.cert.CertificateException;
import java.security.cert.X509Certificate;
import java.security.interfaces.RSAPrivateCrtKey;
import java.security.spec.PKCS8EncodedKeySpec;
import java.security.spec.RSAPublicKeySpec;
import java.util.Date;

public class App {

    static {
        Security.addProvider(new BouncyCastleProvider());
    }

    public static void main(String[] args) {
        try {
            Args parsed = parseArgs(args);
            if (parsed == null) {
                System.err.println("Usage: app -i <pkcs8-private-key.der> -o <output-cert.der>");
                System.exit(1);
            }

            RSAPrivateCrtKey privateKey = readPkcs8PrivateKey(parsed.inputFile);
            X509Certificate cert = generateSelfSignCert(privateKey);
            writeCert(parsed.outputFile, cert);

            System.out.println("Self-signed certificate written to " + parsed.outputFile);
        } catch (Exception e) {
            e.printStackTrace();
            System.exit(1);
        }
    }

    static class Args {
        final String inputFile;
        final String outputFile;

        Args(String inputFile, String outputFile) {
            this.inputFile = inputFile;
            this.outputFile = outputFile;
        }
    }

    public static Args parseArgs(String[] args) {
        String inputFile = null;
        String outputFile = null;

        for (int i = 0; i < args.length; i++) {
            if ("-i".equals(args[i]) && i + 1 < args.length) {
                inputFile = args[i + 1];
                i++;
            } else if ("-o".equals(args[i]) && i + 1 < args.length) {
                outputFile = args[i + 1];
                i++;
            }
        }

        if (inputFile == null || outputFile == null) {
            return null;
        }
        return new Args(inputFile, outputFile);
    }

    public static RSAPrivateCrtKey readPkcs8PrivateKey(String filePath) throws Exception {
        byte[] keyBytes;
        try (FileInputStream fis = new FileInputStream(filePath)) {
            keyBytes = fis.readAllBytes();
        }
        PKCS8EncodedKeySpec keySpec = new PKCS8EncodedKeySpec(keyBytes);
        KeyFactory keyFactory = KeyFactory.getInstance("RSA", "BC");
        return (RSAPrivateCrtKey) keyFactory.generatePrivate(keySpec);
    }

    public static X509Certificate generateSelfSignCert(RSAPrivateCrtKey  rsaPrivateKey) throws Exception {

        RSAPublicKeySpec publicKeySpec = new RSAPublicKeySpec(
                rsaPrivateKey.getModulus(),
                rsaPrivateKey.getPublicExponent()
        );
        KeyFactory keyFactory = KeyFactory.getInstance("RSA", "BC");
        PublicKey publicKey = keyFactory.generatePublic(publicKeySpec);

        // Date
        long now = System.currentTimeMillis();
        Date startDate = new Date(now);
        Date endDate = new Date(now + 365L * 24 * 60 * 60 * 1000);

        // Subject
        // CN -> Common Name
        // OU -> Organization
        // O -> Organization
        // L: Locality (City)
        // ST: State or Province
        // C: Country
        X500Name dnName = new X500Name("CN=SelfSigned, OU=Dev, O=MyCompany, L=Shanghai, ST=Shanghai, C=CN");

        // Serial Number
        BigInteger serialNumber = new BigInteger(64, new SecureRandom());

        X509v3CertificateBuilder certBuilder = new JcaX509v3CertificateBuilder(
                dnName, // issuer
                serialNumber,
                startDate,
                endDate,
                dnName, // subject
                publicKey // subject public key
                );

        ContentSigner contentSigner = new JcaContentSignerBuilder("SHA256WithRSA")
                .setProvider("BC")
                .build(rsaPrivateKey);

        return new JcaX509CertificateConverter()
                .setProvider("BC")
                .getCertificate(certBuilder.build(contentSigner));
    }

    public static void writeCert(String filePath, X509Certificate cert)
            throws IOException, CertificateException {
        try (FileOutputStream fos = new FileOutputStream(filePath)) {
            fos.write(cert.getEncoded());
        }
    }
}
