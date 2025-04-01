
#include <stdio.h>
#include <stdlib.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/x509.h>
#include <openssl/evp.h>
#include <openssl/bio.h>

void handle_openssl_error() {
    ERR_print_errors_fp(stderr);
    exit(EXIT_FAILURE);
}

int compare_public_keys(const char *file1, const char *file2) {
    FILE *fp1 = NULL, *fp2 = NULL;
    EVP_PKEY *key1 = NULL, *key2 = NULL;
    int result = 0;

    // Initialize OpenSSL
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();

    // Read first public key
    fp1 = fopen(file1, "rb");
    if (!fp1) {
        fprintf(stderr, "Error opening file %s\n", file1);
        goto cleanup;
    }
    // Read second public key
    fp2 = fopen(file2, "rb");
    if (!fp2) {
        fprintf(stderr, "Error opening file %s\n", file2);
        goto cleanup;
    }

    // Read DER file directly as public key
    key1 = d2i_PUBKEY_fp(fp1, NULL);
    if (!key1) {
        fprintf(stderr, "Error reading DER public key from %s\n", file1);
        handle_openssl_error();
    }

    key2 = d2i_PUBKEY_fp(fp2, NULL);
    if (!key2) {
        fprintf(stderr, "Error reading DER public key from %s\n", file2);
        handle_openssl_error();
    }

    // Compare the keys
    result = EVP_PKEY_cmp(key1, key2);
    if (result == 1) {
        printf("Public keys are identical\n");
    } else if (result == 0) {
        printf("Public keys are different\n");
    } else {
        fprintf(stderr, "Error comparing public keys\n");
        handle_openssl_error();
    }

cleanup:
    if (key1) EVP_PKEY_free(key1);
    if (key2) EVP_PKEY_free(key2);
    if (fp1) fclose(fp1);
    if (fp2) fclose(fp2);

    return result;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <public_key1.der> <public_key2.der>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int comparison_result = compare_public_keys(argv[1], argv[2]);

    // Clean up OpenSSL
    // ZJW_C EVP_cleanup();
    // ZJW_C CRYPTO_cleanup_all_ex_data();
    // ZJW_C ERR_free_strings();

    return (comparison_result == 1) ? EXIT_SUCCESS : EXIT_FAILURE;
}
