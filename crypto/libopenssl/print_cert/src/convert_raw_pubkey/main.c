#include <stdio.h>
#include <stdlib.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/bio.h>

void handle_openssl_error() {
    ERR_print_errors_fp(stderr);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <pkcs1_public_key_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    FILE *fp = fopen(argv[1], "rb");
    if (!fp) {
        perror("Failed to open input file");
        return EXIT_FAILURE;
    }

    // Initialize OpenSSL
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();

    // Read PKCS#1 public key
    RSA *rsa_key = NULL;
    if (!d2i_RSAPublicKey_fp(fp, &rsa_key)) {
        fclose(fp);
        fprintf(stderr, "Failed to read PKCS#1 public key\n");
        handle_openssl_error();
    }
    fclose(fp);

    // Create EVP_PKEY from RSA key
    EVP_PKEY *pkey = EVP_PKEY_new();
    if (!pkey || !EVP_PKEY_assign_RSA(pkey, rsa_key)) {
        if (pkey) EVP_PKEY_free(pkey);
        RSA_free(rsa_key);
        fprintf(stderr, "Failed to create EVP_PKEY\n");
        handle_openssl_error();
    }

    // Convert to SubjectPublicKeyInfo format (X.509)
    unsigned char *x509_buf = NULL;
    int x509_len = i2d_PUBKEY(pkey, &x509_buf);
    if (x509_len <= 0) {
        EVP_PKEY_free(pkey);
        fprintf(stderr, "Failed to convert to X.509 format\n");
        handle_openssl_error();
    }

    // Output the X.509 public key in PEM format
    BIO *bio = BIO_new_fp(stdout, BIO_NOCLOSE);
    if (!bio) {
        OPENSSL_free(x509_buf);
        EVP_PKEY_free(pkey);
        fprintf(stderr, "Failed to create BIO\n");
        handle_openssl_error();
    }

    printf("X.509 SubjectPublicKeyInfo format (PEM):\n");
    if (!PEM_write_bio_PUBKEY(bio, pkey)) {
        BIO_free(bio);
        OPENSSL_free(x509_buf);
        EVP_PKEY_free(pkey);
        fprintf(stderr, "Failed to write PEM output\n");
        handle_openssl_error();
    }

    // Clean up
    BIO_free(bio);
    OPENSSL_free(x509_buf);
    EVP_PKEY_free(pkey);

    // Clean up OpenSSL
    CRYPTO_cleanup_all_ex_data();
    ERR_free_strings();

    return EXIT_SUCCESS;
}
