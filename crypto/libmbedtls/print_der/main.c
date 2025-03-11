// Claude-3.7 Prompt:
// Please using mbedtls to create a c program,
// read a certificate from file der format. parse it and print certificate info


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mbedtls/x509_crt.h"
#include "mbedtls/error.h"
#include "mbedtls/pk.h"
#include "mbedtls/pem.h"

void print_cert_info(const mbedtls_x509_crt *cert)
{
    char buf[1024];
    size_t n;

    printf("\nCertificate Information:\n");
    printf("------------------------\n");

    // Print subject
    printf("Subject: ");
    mbedtls_x509_dn_gets(buf, sizeof(buf), &cert->subject);
    printf("%s\n", buf);

    // Print issuer
    printf("Issuer: ");
    mbedtls_x509_dn_gets(buf, sizeof(buf), &cert->issuer);
    printf("%s\n", buf);

    // Print serial number
    printf("Serial: ");
    for (n = 0; n < cert->serial.len; n++)
        printf("%02X", cert->serial.p[n]);
    printf("\n");

    // Print validity period
    printf("  Not Before: %04d-%02d-%02d %02d:%02d:%02d UTC\n",
           cert->valid_from.year, cert->valid_from.mon, cert->valid_from.day,
           cert->valid_from.hour, cert->valid_from.min, cert->valid_from.sec);
    printf("  Not After:  %04d-%02d-%02d %02d:%02d:%02d UTC\n",
           cert->valid_to.year, cert->valid_to.mon, cert->valid_to.day,
           cert->valid_to.hour, cert->valid_to.min, cert->valid_to.sec);

    // Print signature algorithm
    printf("Signature Algorithm: ");
    mbedtls_x509_sig_alg_gets(buf, sizeof(buf), &cert->sig_oid, cert->MBEDTLS_PRIVATE(sig_pk), cert->MBEDTLS_PRIVATE(sig_md), cert->MBEDTLS_PRIVATE(sig_opts));
    printf("%s\n", buf);

    // Get and print key type
    mbedtls_pk_type_t pk_type = mbedtls_pk_get_type(&cert->pk);
    printf("Public Key Type: ");
    switch(pk_type) {
        case MBEDTLS_PK_RSA:
            printf("RSA\n");
            break;
        case MBEDTLS_PK_ECKEY:
            printf("Elliptic Curve\n");
            break;
        case MBEDTLS_PK_ECKEY_DH:
            printf("Elliptic Curve Diffie-Hellman\n");
            break;
        case MBEDTLS_PK_ECDSA:
            printf("Elliptic Curve DSA\n");
            break;
        case MBEDTLS_PK_RSA_ALT:
            printf("RSA (Alternative implementation)\n");
            break;
        case MBEDTLS_PK_RSASSA_PSS:
            printf("RSA-PSS\n");
            break;
        default:
            printf("Unknown (%d)\n", pk_type);
    }

    // Print key algorithm name
    printf("Public Key Algorithm: %s\n", mbedtls_pk_get_name(&cert->pk));

    // Print key size
    printf("Public Key Size: %d bits\n", (int) mbedtls_pk_get_bitlen(&cert->pk));

    // Print the actual public key
    printf("Public Key: \n");
    unsigned char key_buf[2048];
    int ret = mbedtls_pk_write_pubkey_pem(&cert->pk, key_buf, sizeof(key_buf));
    if (ret == 0) {
        printf("%s", key_buf);
    } else {
        printf("Failed to extract public key\n");
    }

        // Print the DER format public key in hex
    printf("Public Key (DER Format in Hex):\n");
    unsigned char der_buf[2048];
    ret = mbedtls_pk_write_pubkey_der(&cert->pk, der_buf, sizeof(der_buf));
    if (ret > 0) {
        // mbedtls_pk_write_pubkey_der writes at the end of the buffer!
        unsigned char *start = der_buf + sizeof(der_buf) - ret;
        for (n = 0; n < ret; n++) {
            if (n > 0 && n % 16 == 0) {
                printf("\n");
            }
            printf("0x%02X ", start[n]);
        }
        printf("\n");
    } else {
        printf("Failed to extract public key in DER format\n");
    }


    // Print certificate version
    printf("Version: %d\n", cert->version);

    // Print fingerprint
    unsigned char hash[32];
    printf("SHA-256 Fingerprint: ");
    if (mbedtls_md(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256),
        cert->raw.p, cert->raw.len, hash) == 0) {
        for (n = 0; n < 32; n++)
            printf("%02X%s", hash[n], (n < 31) ? ":" : "");
        printf("\n");
    }
}

int main(int argc, char *argv[])
{
    mbedtls_x509_crt cert;
    int ret;

    if (argc != 2) {
        printf("Usage: %s <certificate-file.der>\n", argv[0]);
        return 1;
    }

    mbedtls_x509_crt_init(&cert);

    // Read and parse DER certificate
    ret = mbedtls_x509_crt_parse_file(&cert, argv[1]);
    if (ret != 0) {
        char error_buf[100];
        mbedtls_strerror(ret, error_buf, sizeof(error_buf));
        printf("Failed to parse certificate: %s\n", error_buf);
        mbedtls_x509_crt_free(&cert);
        return 1;
    }

    // Print certificate information
    print_cert_info(&cert);

    // Clean up
    mbedtls_x509_crt_free(&cert);

    return 0;
}
