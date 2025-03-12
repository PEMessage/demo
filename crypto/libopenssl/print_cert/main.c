#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <stdio.h>
#include <stdlib.h>

void handle_errors() {
    ERR_print_errors_fp(stderr);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <certificate_file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *cert_file = argv[1];
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();

    FILE *fp = fopen(cert_file, "rb");
    if (!fp) {
        perror("Failed to open certificate file");
        exit(EXIT_FAILURE);
    }

    X509 *cert = PEM_read_X509(fp, NULL, NULL, NULL);
    if (!cert) {
        fseek(fp, 0, SEEK_SET);
        cert = d2i_X509_fp(fp, NULL);
        if (!cert) {
            handle_errors();
        }
    }
    fclose(fp);

    EVP_PKEY *pkey = X509_get_pubkey(cert);
    if (!pkey) {
        handle_errors();
    }

    // PEM format output
    BIO *pem_bio = BIO_new(BIO_s_mem());
    if (!PEM_write_bio_PUBKEY(pem_bio, pkey)) {
        handle_errors();
    }

    char *pem_data;
    long pem_len = BIO_get_mem_data(pem_bio, &pem_data);
    printf("PEM format:\n%.*s", (int)pem_len, pem_data);
    BIO_free(pem_bio);

    // DER format output
    unsigned char *der_data = NULL;
    int der_len = i2d_PUBKEY(pkey, &der_data);
    if (der_len <= 0) {
        handle_errors();
    }

    printf("DER format in hex:\n");
    for (int i = 0; i < der_len; i++) {
        printf("0x%02X ", der_data[i]);
        if ( i % 8 == 7 ) printf("\n");
        // if (i < der_len - 1) printf(" ");
    }
    printf("\n");

    OPENSSL_free(der_data);
    EVP_PKEY_free(pkey);
    X509_free(cert);
    EVP_cleanup();
    ERR_free_strings();

    return 0;
}
