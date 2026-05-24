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
import java.math.BigInteger;
import java.security.KeyFactory;
import java.security.PrivateKey;
import java.security.PublicKey;
import java.security.Security;
import java.security.cert.X509Certificate;
import java.security.interfaces.RSAPrivateCrtKey;
import java.security.spec.PKCS8EncodedKeySpec;
import java.security.spec.RSAPublicKeySpec;
import java.util.Date;

public class App {
    public static void main(String[] args) {
        try {
            Security.addProvider(new BouncyCastleProvider());

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
                System.err.println("Usage: app -i <pkcs8-private-key.der> -o <output-cert.der>");
                System.exit(1);
            }

            // Read PKCS#8 private key
            byte[] keyBytes;
            try (FileInputStream fis = new FileInputStream(inputFile)) {
                keyBytes = fis.readAllBytes();
            }

            PKCS8EncodedKeySpec keySpec = new PKCS8EncodedKeySpec(keyBytes);
            KeyFactory keyFactory = KeyFactory.getInstance("RSA", "BC");
            PrivateKey privateKey = keyFactory.generatePrivate(keySpec);

            // Extract public key from RSA private key
            RSAPrivateCrtKey rsaPrivateKey = (RSAPrivateCrtKey) privateKey;
            RSAPublicKeySpec publicKeySpec = new RSAPublicKeySpec(
                rsaPrivateKey.getModulus(),
                rsaPrivateKey.getPublicExponent()
            );
            PublicKey publicKey = keyFactory.generatePublic(publicKeySpec);

            // Build self-signed X.509 certificate
            long now = System.currentTimeMillis();
            Date startDate = new Date(now);
            Date endDate = new Date(now + 365 * 24 * 60 * 60 * 1000L); // 1 year validity

            X500Name dnName = new X500Name("CN=SelfSigned");
            BigInteger serialNumber = BigInteger.valueOf(now);

            X509v3CertificateBuilder certBuilder = new JcaX509v3CertificateBuilder(
                dnName, serialNumber, startDate, endDate, dnName, publicKey
            );

            ContentSigner contentSigner = new JcaContentSignerBuilder("SHA256WithRSA")
                .setProvider("BC")
                .build(privateKey);

            X509Certificate cert = new JcaX509CertificateConverter()
                .setProvider("BC")
                .getCertificate(certBuilder.build(contentSigner));

            // Write certificate in DER format
            try (FileOutputStream fos = new FileOutputStream(outputFile)) {
                fos.write(cert.getEncoded());
            }

            System.out.println("Self-signed certificate written to " + outputFile);

        } catch (Exception e) {
            e.printStackTrace();
            System.exit(1);
        }
    }
}
