#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/rsa.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#include <openssl/objects.h>

void handle_openssl_error() {
    ERR_print_errors_fp(stderr);
    exit(EXIT_FAILURE);
}

unsigned char *read_file_to_buffer(const char *filename, size_t *out_len) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("Failed to open input file");
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (file_size <= 0) {
        fclose(fp);
        fprintf(stderr, "Invalid file size\n");
        return NULL;
    }

    unsigned char *buffer = malloc(file_size);
    if (!buffer) {
        fclose(fp);
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }

    if (fread(buffer, 1, file_size, fp) != (size_t)file_size) {
        fclose(fp);
        free(buffer);
        fprintf(stderr, "Failed to read file\n");
        return NULL;
    }

    fclose(fp);
    *out_len = (size_t)file_size;
    return buffer;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <pkcs1_public_key_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Initialize OpenSSL
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();

    // Read file into buffer
    size_t file_len = 0;
    unsigned char *file_buf = read_file_to_buffer(argv[1], &file_len);
    if (!file_buf) {
        return EXIT_FAILURE;
    }

    // Parse PKCS#1 public key using d2i_PublicKey
    const unsigned char *p = file_buf;
    EVP_PKEY *pkey = d2i_PublicKey(EVP_PKEY_RSA, NULL, &p, (long)file_len);
    if (!pkey) {
        free(file_buf);
        fprintf(stderr, "Failed to parse PKCS#1 public key\n");
        handle_openssl_error();
    }

    // Convert to X.509 SubjectPublicKeyInfo format
    unsigned char *x509_buf = NULL;
    int x509_len = i2d_PUBKEY(pkey, &x509_buf);
    if (x509_len <= 0) {
        free(file_buf);
        EVP_PKEY_free(pkey);
        fprintf(stderr, "Failed to convert to X.509 format\n");
        handle_openssl_error();
    }

    // Output the X.509 public key in PEM format
    BIO *bio_out = BIO_new_fp(stdout, BIO_NOCLOSE);
    if (!bio_out) {
        free(file_buf);
        OPENSSL_free(x509_buf);
        EVP_PKEY_free(pkey);
        fprintf(stderr, "Failed to create BIO\n");
        handle_openssl_error();
    }

    printf("X.509 SubjectPublicKeyInfo format (PEM):\n");
    if (!PEM_write_bio_PUBKEY(bio_out, pkey)) {
        BIO_free(bio_out);
        free(file_buf);
        OPENSSL_free(x509_buf);
        EVP_PKEY_free(pkey);
        fprintf(stderr, "Failed to write PEM output\n");
        handle_openssl_error();
    }

    // Clean up
    BIO_free(bio_out);
    free(file_buf);
    OPENSSL_free(x509_buf);
    EVP_PKEY_free(pkey);

    // Clean up OpenSSL
    CRYPTO_cleanup_all_ex_data();
    ERR_free_strings();

    return EXIT_SUCCESS;
}
