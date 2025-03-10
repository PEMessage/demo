// QwQ-32b Prompt:
// Please using mbedtls to create a c program,
// read a certificate from file der format. parse it and print certificate info

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mbedtls/x509.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/error.h>


int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <certificate.der>\n", argv[0]);
        return 1;
    }

    const char *filename = argv[1];
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Failed to open file");
        return 1;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    unsigned char *buffer = (unsigned char*)malloc(file_size);
    if (!buffer) {
        fclose(file);
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }

    if (fread(buffer, 1, file_size, file) != (size_t)file_size) {
        free(buffer);
        fclose(file);
        fprintf(stderr, "Failed to read file\n");
        return 1;
    }
    fclose(file);

    mbedtls_x509_crt crt;
    mbedtls_x509_crt_init(&crt);

    int ret = mbedtls_x509_crt_parse(&crt, buffer, file_size);
    free(buffer);

    if (ret != 0) {
        char error_buf[100];
        mbedtls_strerror(ret, error_buf, sizeof(error_buf));
        fprintf(stderr, "Parsing failed: %s\n", error_buf);
        mbedtls_x509_crt_free(&crt);
        return 1;
    }

    // Print certificate information
    char subject[1024], issuer[1024];
    mbedtls_x509_dn_gets(subject, sizeof(subject), &crt.subject);
    mbedtls_x509_dn_gets(issuer, sizeof(issuer), &crt.issuer);
    printf("Subject: %s\n", subject);
    printf("Issuer: %s\n", issuer);

    printf("Valid From: %d-%d-%d UTC\n", crt.valid_from.year, crt.valid_from.mon, crt.valid_from.day);
    printf("Valid to:   %d-%d-%d UTC\n",   crt.valid_to.year,   crt.valid_to.mon,   crt.valid_to.day);

    char serial[100];
    // mbedtls_mpi_write_string(&crt.serial, 16, serial, sizeof(serial), NULL);
    // printf("Serial Number: %s\n", serial);

    printf("Version: %d\n", crt.version + 1); // X.509 versions are 1-based

    // Public key information
    mbedtls_pk_type_t pk_type = mbedtls_pk_get_type(&crt.pk);
    const char *pk_alg = "Unknown";
    int key_size = 0;

    switch (pk_type) {
        case MBEDTLS_PK_RSA:
            pk_alg = "RSA";
            key_size = mbedtls_rsa_get_len(mbedtls_pk_rsa(crt.pk));
            break;
        case MBEDTLS_PK_ECKEY:
            pk_alg = "EC";
            {
                mbedtls_ecp_keypair *ec = mbedtls_pk_ec(crt.pk);
                const mbedtls_ecp_curve_info *curve = mbedtls_ecp_curve_info_from_grp_id(
                    mbedtls_ecp_keypair_get_group_id(ec)
                );
                if (curve) {
                    printf("EC Curve: %s\n", curve->name);
                }
            }
            break;
        default:
            break;
    }

    if (key_size > 0) {
        printf("Public Key Algorithm: %s (%d bits)\n", pk_alg, key_size);
    }

    mbedtls_x509_crt_free(&crt);
    return 0;
}
