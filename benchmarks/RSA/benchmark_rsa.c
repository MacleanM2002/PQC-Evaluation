#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/obj_mac.h>
#include <openssl/pem.h>  // For saving PEM files
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>

#define ITERATIONS 1000
#define OUTPUT_DIR "results/classical_rsa"
#define OUTPUT_FILE "results/classical_rsa/benchmark_rsa.csv"
#define KEY_FILE "certs/rsa_key.pem"  // Specify your desired key file name here

double time_diff_ms(struct timeval start, struct timeval end) {
    return (end.tv_sec - start.tv_sec) * 1000.0 +
           (end.tv_usec - start.tv_usec) / 1000.0;
}

void ensure_output_dir() {
    struct stat st = {0};
    if (stat(OUTPUT_DIR, &st) == -1) {
        if (mkdir(OUTPUT_DIR, 0755) != 0) {
            perror("Failed to create output directory");
            exit(1);
        }
    }
}

void benchmark_rsa() {
    ensure_output_dir();

    FILE *fp = fopen(OUTPUT_FILE, "w");
    if (!fp) {
        perror("Unable to open output file");
        exit(1);
    }
    fprintf(fp, "operation,time_ms\n");

    for (int i = 0; i < ITERATIONS; i++) {
        struct timeval start, end;

        EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);
        if (!ctx) {
            fprintf(stderr, "RSA context creation failed at iteration %d\n", i);
            continue;
        }

        if (EVP_PKEY_keygen_init(ctx) <= 0 ||
            EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, 3072) <= 0) {
            fprintf(stderr, "RSA keygen init failed at iteration %d\n", i);
            EVP_PKEY_CTX_free(ctx);
            continue;
        }

        EVP_PKEY *key = NULL;
        gettimeofday(&start, NULL);
        if (EVP_PKEY_keygen(ctx, &key) <= 0) {
            fprintf(stderr, "RSA keygen failed at iteration %d\n", i);
            EVP_PKEY_CTX_free(ctx);
            continue;
        }
        gettimeofday(&end, NULL);

        double elapsed_ms = time_diff_ms(start, end);
        fprintf(fp, "keygen,%.3f\n", elapsed_ms);

        // Save the generated RSA key to a file in PEM format
        FILE *key_file = fopen(KEY_FILE, "wb");
        if (!key_file) {
            perror("Unable to open key file for writing");
            EVP_PKEY_free(key);
            EVP_PKEY_CTX_free(ctx);
            fclose(fp);
            exit(1);
        }

        // Write the private key to the file in PEM format
        if (PEM_write_PrivateKey(key_file, key, NULL, NULL, 0, NULL, NULL) <= 0) {
            fprintf(stderr, "Failed to write private key to PEM file\n");
            fclose(key_file);
            EVP_PKEY_free(key);
            EVP_PKEY_CTX_free(ctx);
            fclose(fp);
            exit(1);
        }
        fclose(key_file);  // Close the file after saving the key

        EVP_PKEY_free(key);
        EVP_PKEY_CTX_free(ctx);
    }

    fclose(fp);
    printf("[✓] RSA benchmark complete. Output saved to %s\n", OUTPUT_FILE);
    printf("[✓] RSA private key saved to %s\n", KEY_FILE);
}

int main() {
    OPENSSL_init_crypto(OPENSSL_INIT_LOAD_CRYPTO_STRINGS, NULL);
    OpenSSL_add_all_algorithms();

    benchmark_rsa();

    EVP_cleanup();
    ERR_free_strings();
    return 0;
}
