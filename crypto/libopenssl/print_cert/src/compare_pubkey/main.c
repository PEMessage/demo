#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void handle_openssl_error() {
    ERR_print_errors_fp(stderr);
    exit(EXIT_FAILURE);
}

EVP_PKEY *load_pubkey(const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("fopen failed");
        return NULL;
    }

    // Read entire file
    fseek(fp, 0, SEEK_END);
    long file_len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    unsigned char *file_buf = malloc(file_len);
    if (!file_buf) {
        perror("malloc failed");
        fclose(fp);
        return NULL;
    }
    
    if (fread(file_buf, 1, file_len, fp) != (size_t)file_len) {
        perror("fread failed");
        free(file_buf);
        fclose(fp);
        return NULL;
    }
    fclose(fp);

    const unsigned char *p = file_buf;
    EVP_PKEY *pkey = NULL;

    // First try d2i_PUBKEY (expect a SubjectPublicKeyInfo format)
    pkey = d2i_PUBKEY(NULL, &p, file_len);
    if (pkey != NULL) {
        free(file_buf);
        return pkey;
    }
    ERR_clear_error();
    ERR_print_errors_fp(stderr);

    // If that failed, reset and try PKCS#1 / PKCS#8 format for RSA
    // Doc:
    //      i2d_PrivateKey() encodes a. It uses a key specific format or, 
    //      if none is defined for that key type, PKCS#8 unencrypted PrivateKeyInfo format.
    //      i2d_PublicKey() does the same for public keys.
    // Different about d2i_PublicKey / d2i_PUBKEY : Also See:
    //      https://stackoverflow.com/questions/67717197/what-is-the-difference-between-openssl-functions-d2i-publickey-d2i-pubkey-an
    //      https://stackoverflow.com/questions/76683427/extracting-the-encoded-public-key-from-rsa-key-pair-in-openssl-3/76684263#76684263
    p = file_buf;
    pkey = d2i_PublicKey(EVP_PKEY_RSA, NULL, &p, file_len);
    if (pkey != NULL) {
        free(file_buf);
        return pkey;
    }

    // If that failed, reset and try PKCS#1 format for EC
    // Update: PKCS#1 is RSA only 
    //  There is no such thing as an "EC key in PKCS#1 format":
    //  PKCS#1 is only for RSA keys, not EC keys.
    //  However, there is another format, analogous to PKCS#1 but made for EC keys, and defined in SEC 1.
    //  OpenSSL can convert that format into the generic PKCS#8 with the "openssl pkcs8" command, and back into SEC 1 format with "openssl ec".
    // Also See:
    //  https://security.stackexchange.com/questions/84327/converting-ecc-private-key-to-pkcs1-format
#if 0
    // Err:
    //  To decode a key with type EVP_PKEY_EC,
    //  d2i_PublicKey() requires *a to be a non-NULL EVP_PKEY structure assigned an EC_KEY structure referencing the proper EC_GROUP.
    // Also See: openssl document
    ERR_clear_error();
    p = file_buf;
    pkey = d2i_PublicKey(EVP_PKEY_EC, NULL, &p, file_len);
#endif

    free(file_buf);
    return pkey;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <pubkey1> <pubkey2>\n", argv[0]);
        return EXIT_FAILURE;
    }

    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();

    EVP_PKEY *pkey1 = load_pubkey(argv[1]);
    if (!pkey1) {
        fprintf(stderr, "Failed to load first public key\n");
        handle_openssl_error();
    }

    EVP_PKEY *pkey2 = load_pubkey(argv[2]);
    if (!pkey2) {
        fprintf(stderr, "Failed to load second public key\n");
        EVP_PKEY_free(pkey1);
        handle_openssl_error();
    }

    int cmp_result = EVP_PKEY_cmp(pkey1, pkey2);
    EVP_PKEY_free(pkey1);
    EVP_PKEY_free(pkey2);

    if (cmp_result == 1) {
        printf("Public keys are equal\n");
        return EXIT_SUCCESS;
    } else if (cmp_result == 0) {
        printf("Public keys are different\n");
        return EXIT_SUCCESS;
    } else {
        fprintf(stderr, "Error comparing public keys\n");
        handle_openssl_error();
    }

    return EXIT_SUCCESS;
}
