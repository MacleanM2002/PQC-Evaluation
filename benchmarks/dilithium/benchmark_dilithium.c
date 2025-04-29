#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/obj_mac.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>

#define ITERATIONS 1000
#define OUTPUT_DIR "results/dilithium"
#define OUTPUT_FILE "results/dilithium/benchmark_dilithium.csv"

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

void benchmark_dilithium() {
    ensure_output_dir();

    FILE *fp = fopen(OUTPUT_FILE, "w");
    if (!fp) {
        perror("Unable to open output file");
        exit(1);
    }
    fprintf(fp, "operation,time_ms\n");

    // Use "mldsa44" as the new EVP name for Dilithium2.
    int nid = OBJ_txt2nid("mldsa44");
    if (nid == NID_undef) {
        fprintf(stderr, "Error: Could not resolve NID for mldsa44\n");
        ERR_print_errors_fp(stderr);
        fclose(fp);
        exit(1);
    }

    for (int i = 0; i < ITERATIONS; i++) {
        struct timeval start, end;

        EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new_id(nid, NULL);
        if (!ctx) {
            fprintf(stderr, "EVP_PKEY_CTX_new_id failed at iteration %d\n", i);
            ERR_print_errors_fp(stderr);
            continue;
        }

        if (EVP_PKEY_keygen_init(ctx) <= 0) {
            fprintf(stderr, "EVP_PKEY_keygen_init failed at iteration %d\n", i);
            ERR_print_errors_fp(stderr);
            EVP_PKEY_CTX_free(ctx);
            continue;
        }

        EVP_PKEY *key = NULL;
        gettimeofday(&start, NULL);
        if (EVP_PKEY_keygen(ctx, &key) <= 0) {
            fprintf(stderr, "EVP_PKEY_keygen failed at iteration %d\n", i);
            ERR_print_errors_fp(stderr);
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
    printf("[✓] Dilithium (mldsa44) benchmark complete. Output saved to %s\n", OUTPUT_FILE);
}

int main() {
    OPENSSL_init_crypto(OPENSSL_INIT_LOAD_CRYPTO_STRINGS, NULL);
    OpenSSL_add_all_algorithms();

    benchmark_dilithium();

    EVP_cleanup();
    ERR_free_strings();
    return 0;
}
