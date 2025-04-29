#!/bin/bash
set -euo pipefail

echo "[*] Collecting Dilithium benchmark..."

cd "$(dirname "$0")/.." || exit 1

# --- Environment setup ---
export OPENSSL="/opt/openssl/bin/openssl"
export LD_LIBRARY_PATH="/opt/openssl/lib64:$LD_LIBRARY_PATH"
export OPENSSL_MODULES="/opt/oqs-provider/lib"
export OPENSSL_CONF="/opt/openssl/openssl.cnf"

SRC="benchmarks/dilithium/benchmark_dilithium.c"
BIN="binaries/dilithium/benchmark_dilithium"
OUT="results/dilithium/benchmark_dilithium.csv"

# Ensure GCC is available
if ! command -v gcc &> /dev/null; then
    echo "[!] gcc is not installed or not in PATH."
    exit 1
fi

# Create required directories for the binary and output file
mkdir -p "$(dirname "$BIN")"
mkdir -p "$(dirname "$OUT")"

# --- Certificate and Key Generation for Dilithium ---
CERT="certs/dilithium_cert.pem"
KEY="certs/dilithium_key.pem"

# If the certificate or key do not exist, generate them.
if [ ! -f "$CERT" ] || [ ! -f "$KEY" ]; then
    echo "[*] Generating Dilithium certificate and key..."
    # Using a hybrid key type (p256_mldsa44) for better compatibility with TLS
    openssl req -x509 -days 365 -new -newkey p256_mldsa44 -nodes \
      -keyout "$KEY" -out "$CERT" -subj "/CN=localhost"
fi

# --- Compile the Dilithium Benchmark ---
echo "[*] Compiling Dilithium benchmark..."
gcc "$SRC" -o "$BIN" -lcrypto -lrt

# --- Run the Dilithium Benchmark ---
echo "[*] Running Dilithium benchmark..."
start_time=$(date +%s)

if ! "$BIN"; then
    echo "[!] Error running Dilithium benchmark binary."
    exit 1
fi

end_time=$(date +%s)
runtime=$((end_time - start_time))

if [ -f "$OUT" ]; then
    echo "[✓] Dilithium results saved to $OUT"
    echo "[⏱] Runtime: ${runtime} seconds"
else
    echo "[!] Benchmark did not produce an output file: $OUT"
    exit 1
fi
