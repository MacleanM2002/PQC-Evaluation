#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/provider.h>  // Correct header for OpenSSL provider functions
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>

#define ITERATIONS 1000
#define OUTPUT_DIR "results/kyber"
#define OUTPUT_FILE "results/kyber/benchmark_kyber.csv"

/* Use static to give internal linkage and avoid symbol collisions */
static double kyber_time_diff_ms(struct timeval start, struct timeval end) {
    return (end.tv_sec - start.tv_sec) * 1000.0 +
           (end.tv_usec - start.tv_usec) / 1000.0;
}

static void kyber_ensure_output_dir(void) {
    struct stat st;
    if (stat(OUTPUT_DIR, &st) == -1) {
        if (mkdir(OUTPUT_DIR, 0755) != 0) {
            perror("Failed to create output directory");
            exit(1);
        }
    }
}

static void kyber_benchmark(void) {
    kyber_ensure_output_dir();

    FILE *fp = fopen(OUTPUT_FILE, "w");
    if (!fp) {
        perror("Unable to open output file");
        exit(1);
    }
    fprintf(fp, "operation,time_ms\n");

    // Load OQS provider (this is needed for the PQC algorithms)
    if (OSSL_PROVIDER_load(NULL, "oqs") == NULL) {
        fprintf(stderr, "Failed to load OQS provider.\n");
        fclose(fp);
        return;  // Return without a value in a void function
    }

    // Initialize EVP_PKEY for the Kyber algorithm
    EVP_PKEY *pkey = EVP_PKEY_new();
    if (!pkey) {
        fprintf(stderr, "Failed to create EVP_PKEY object.\n");
        fclose(fp);
        return;  // Return without a value in a void function
    }

    // Initialize key generation context for Kyber (mlkem512)
    EVP_PKEY_CTX *gen_ctx = EVP_PKEY_CTX_new_from_name(NULL, "mlkem512", NULL); // Use the algorithm name directly
    if (!gen_ctx) {
        fprintf(stderr, "Failed to create EVP_PKEY_CTX for Kyber algorithm.\n");
        EVP_PKEY_free(pkey);
        fclose(fp);
        return;
    }

    // Generate the key pair for Kyber (mlkem512)
    if (EVP_PKEY_keygen_init(gen_ctx) <= 0 || EVP_PKEY_keygen(gen_ctx, &pkey) <= 0) {
        fprintf(stderr, "Key generation for mlkem512 failed\n");
        ERR_print_errors_fp(stderr);
        EVP_PKEY_CTX_free(gen_ctx);
        EVP_PKEY_free(pkey);
        fclose(fp);
        return;  // Return without a value in a void function
    }
    EVP_PKEY_CTX_free(gen_ctx);

    size_t ct_len = 0, ss_len = 0;
    EVP_PKEY_CTX *enc_ctx = EVP_PKEY_CTX_new(pkey, NULL);
    if (!enc_ctx || EVP_PKEY_encapsulate_init(enc_ctx, NULL) <= 0) {
        fprintf(stderr, "Encapsulation context init failed\n");
        ERR_print_errors_fp(stderr);
        EVP_PKEY_CTX_free(enc_ctx);
        EVP_PKEY_free(pkey);
        fclose(fp);
        return;
    }

    // Calculate ciphertext and shared secret lengths
    if (EVP_PKEY_encapsulate(enc_ctx, NULL, &ct_len, NULL, &ss_len) <= 0) {
        fprintf(stderr, "Determining ciphertext and shared secret lengths failed\n");
        ERR_print_errors_fp(stderr);
        EVP_PKEY_CTX_free(enc_ctx);
        EVP_PKEY_free(pkey);
        fclose(fp);
        return;
    }

    unsigned char *ciphertext = malloc(ct_len);
    if (!ciphertext) {
        perror("malloc ciphertext");
        EVP_PKEY_CTX_free(enc_ctx);
        EVP_PKEY_free(pkey);
        fclose(fp);
        return;
    }

    unsigned char *shared_secret_enc = malloc(ss_len);
    unsigned char *shared_secret_dec = malloc(ss_len);
    if (!shared_secret_enc || !shared_secret_dec) {
        perror("malloc shared secret");
        free(ciphertext);
        if (shared_secret_enc) free(shared_secret_enc);
        EVP_PKEY_CTX_free(enc_ctx);
        EVP_PKEY_free(pkey);
        fclose(fp);
        return;
    }
    EVP_PKEY_CTX_free(enc_ctx);

    double total_enc_ms = 0.0, total_dec_ms = 0.0;
    for (int i = 0; i < ITERATIONS; i++) {
        struct timeval start, end;

        /* Encapsulation: create a new context each iteration */
        EVP_PKEY_CTX *ctx_enc = EVP_PKEY_CTX_new(pkey, NULL);
        if (!ctx_enc || EVP_PKEY_encapsulate_init(ctx_enc, NULL) <= 0) {
            fprintf(stderr, "Encapsulation context reinit failed at iteration %d\n", i);
            ERR_print_errors_fp(stderr);
            EVP_PKEY_CTX_free(ctx_enc);
            break;
        }
        gettimeofday(&start, NULL);
        if (EVP_PKEY_encapsulate(ctx_enc, ciphertext, &ct_len, shared_secret_enc, &ss_len) <= 0) {
            fprintf(stderr, "Encapsulation failed at iteration %d\n", i);
            ERR_print_errors_fp(stderr);
            EVP_PKEY_CTX_free(ctx_enc);
            break;
        }
        gettimeofday(&end, NULL);
        double enc_time = kyber_time_diff_ms(start, end);
        total_enc_ms += enc_time;
        fprintf(fp, "encaps,%.3f\n", enc_time);
        EVP_PKEY_CTX_free(ctx_enc);

        /* Decapsulation: create a new context each iteration */
        EVP_PKEY_CTX *ctx_dec = EVP_PKEY_CTX_new(pkey, NULL);
        if (!ctx_dec || EVP_PKEY_decapsulate_init(ctx_dec, NULL) <= 0) {
            fprintf(stderr, "Decapsulation context init failed at iteration %d\n", i);
            ERR_print_errors_fp(stderr);
            EVP_PKEY_CTX_free(ctx_dec);
            break;
        }
        gettimeofday(&start, NULL);
        if (EVP_PKEY_decapsulate(ctx_dec, shared_secret_dec, &ss_len, ciphertext, ct_len) <= 0) {
            fprintf(stderr, "Decapsulation failed at iteration %d\n", i);
            ERR_print_errors_fp(stderr);
            EVP_PKEY_CTX_free(ctx_dec);
            break;
        }
        gettimeofday(&end, NULL);
        double dec_time = kyber_time_diff_ms(start, end);
        total_dec_ms += dec_time;
        fprintf(fp, "decaps,%.3f\n", dec_time);
        EVP_PKEY_CTX_free(ctx_dec);
    }

    printf("[✓] Kyber (mlkem512) benchmark complete.\n");
    printf("Average encapsulation time: %.3f ms\n", total_enc_ms / ITERATIONS);
    printf("Average decapsulation time: %.3f ms\n", total_dec_ms / ITERATIONS);

    free(ciphertext);
    free(shared_secret_enc);
    free(shared_secret_dec);
    EVP_PKEY_free(pkey);
    fclose(fp);
}

int main(void) {
    /* Initialize error strings and algorithms. */
    ERR_load_crypto_strings();
    OpenSSL_add_all_algorithms();

    /* Run the benchmark */
    kyber_benchmark();

    EVP_cleanup();
    ERR_free_strings();
    return 0;
}
