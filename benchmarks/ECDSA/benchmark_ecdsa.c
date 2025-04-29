#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/obj_mac.h>
#include <openssl/ec.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>

#define ITERATIONS 1000
#define OUTPUT_DIR "results/classical_ecdsa"
#define OUTPUT_FILE "results/classical_ecdsa/benchmark_ecdsa.csv"

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

void benchmark_ecdsa() {
    ensure_output_dir();

    FILE *fp = fopen(OUTPUT_FILE, "w");
    if (!fp) {
        perror("Unable to open output file");
        exit(1);
    }
    fprintf(fp, "operation,time_ms\n");

    for (int i = 0; i < ITERATIONS; i++) {
        struct timeval start, end;

        EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, NULL);
        if (!ctx) {
            fprintf(stderr, "ECDSA context creation failed at iteration %d\n", i);
            continue;
        }

        if (EVP_PKEY_keygen_init(ctx) <= 0 ||
            EVP_PKEY_CTX_set_ec_paramgen_curve_nid(ctx, NID_X9_62_prime256v1) <= 0) {
            fprintf(stderr, "ECDSA keygen init failed at iteration %d\n", i);
            EVP_PKEY_CTX_free(ctx);
            continue;
        }

        EVP_PKEY *key = NULL;
        gettimeofday(&start, NULL);
        if (EVP_PKEY_keygen(ctx, &key) <= 0) {
            fprintf(stderr, "ECDSA keygen failed at iteration %d\n", i);
            EVP_PKEY_CTX_free(ctx);
            continue;
        }
        gettimeofday(&end, NULL);

        double elapsed = time_diff_ms(start, end);
        fprintf(fp, "keygen,%.3f\n", elapsed);

        EVP_PKEY_free(key);
        EVP_PKEY_CTX_free(ctx);
    }

    fclose(fp);
    printf("[✓] ECDSA benchmark complete. Output saved to %s\n", OUTPUT_FILE);
}

int main() {
    OPENSSL_init_crypto(OPENSSL_INIT_LOAD_CRYPTO_STRINGS, NULL);
    OpenSSL_add_all_algorithms();

    benchmark_ecdsa();

    EVP_cleanup();
    ERR_free_strings();
    return 0;
}
