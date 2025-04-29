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
#define OUTPUT_DIR "results/classical_ecdh"
#define OUTPUT_FILE "results/classical_ecdh/benchmark_ecdh.csv"

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

void benchmark_ecdh_shared_secret() {
    ensure_output_dir();

    FILE *fp = fopen(OUTPUT_FILE, "w");
    if (!fp) {
        perror("Unable to open output file");
        exit(1);
    }
    fprintf(fp, "operation,time_ms\n");

    for (int i = 0; i < ITERATIONS; i++) {
        struct timeval start, end;
        EVP_PKEY *pkeyA = NULL, *pkeyB = NULL;
        EVP_PKEY_CTX *ctxA = NULL, *ctxB = NULL, *derive_ctx = NULL;
        unsigned char *secret = NULL;
        size_t secret_len = 0;

        // Generate key pair for party A.
        ctxA = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, NULL);
        if (!ctxA ||
            EVP_PKEY_keygen_init(ctxA) <= 0 ||
            EVP_PKEY_CTX_set_ec_paramgen_curve_nid(ctxA, NID_X9_62_prime256v1) <= 0 ||
            EVP_PKEY_keygen(ctxA, &pkeyA) <= 0) {
            fprintf(stderr, "Key generation failed for party A at iteration %d\n", i);
            ERR_print_errors_fp(stderr);
            goto cleanup;
        }

        // Generate key pair for party B.
        ctxB = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, NULL);
        if (!ctxB ||
            EVP_PKEY_keygen_init(ctxB) <= 0 ||
            EVP_PKEY_CTX_set_ec_paramgen_curve_nid(ctxB, NID_X9_62_prime256v1) <= 0 ||
            EVP_PKEY_keygen(ctxB, &pkeyB) <= 0) {
            fprintf(stderr, "Key generation failed for party B at iteration %d\n", i);
            ERR_print_errors_fp(stderr);
            goto cleanup;
        }

        // Set up derivation context using party A's key.
        derive_ctx = EVP_PKEY_CTX_new(pkeyA, NULL);
        if (!derive_ctx ||
            EVP_PKEY_derive_init(derive_ctx) <= 0 ||
            EVP_PKEY_derive_set_peer(derive_ctx, pkeyB) <= 0) {
            fprintf(stderr, "Derivation context setup failed at iteration %d\n", i);
            ERR_print_errors_fp(stderr);
            goto cleanup;
        }

        // Determine buffer length.
        if (EVP_PKEY_derive(derive_ctx, NULL, &secret_len) <= 0) {
            fprintf(stderr, "Determining secret length failed at iteration %d\n", i);
            ERR_print_errors_fp(stderr);
            goto cleanup;
        }

        secret = (unsigned char *)malloc(secret_len);
        if (!secret) {
            perror("Unable to allocate memory for secret");
            goto cleanup;
        }

        // Benchmark the shared secret derivation.
        gettimeofday(&start, NULL);
        if (EVP_PKEY_derive(derive_ctx, secret, &secret_len) <= 0) {
            fprintf(stderr, "Shared secret derivation failed at iteration %d\n", i);
            ERR_print_errors_fp(stderr);
            goto cleanup;
        }
        gettimeofday(&end, NULL);

        double elapsed = time_diff_ms(start, end);
        fprintf(fp, "derive,%.3f\n", elapsed);

    cleanup:
        if (secret) {
            free(secret);
            secret = NULL;
        }
        if (derive_ctx) {
            EVP_PKEY_CTX_free(derive_ctx);
            derive_ctx = NULL;
        }
        if (pkeyA) {
            EVP_PKEY_free(pkeyA);
            pkeyA = NULL;
        }
        if (pkeyB) {
            EVP_PKEY_free(pkeyB);
            pkeyB = NULL;
        }
        if (ctxA) {
            EVP_PKEY_CTX_free(ctxA);
            ctxA = NULL;
        }
        if (ctxB) {
            EVP_PKEY_CTX_free(ctxB);
            ctxB = NULL;
        }
    }

    fclose(fp);
    printf("[✓] ECDH shared secret benchmark complete. Output saved to %s\n", OUTPUT_FILE);
}

int main() {
    OPENSSL_init_crypto(OPENSSL_INIT_LOAD_CRYPTO_STRINGS, NULL);
    OpenSSL_add_all_algorithms();

    benchmark_ecdh_shared_secret();

    EVP_cleanup();
    ERR_free_strings();
    return 0;
}
