#!/bin/bash
set -euo pipefail

echo "[*] Collecting Kyber benchmark..."

cd "$(dirname "$0")/.." || exit 1

# --- Environment setup ---
export OPENSSL="/opt/openssl/bin/openssl"
export LD_LIBRARY_PATH="/opt/openssl/lib64:$LD_LIBRARY_PATH"
export OPENSSL_MODULES="/opt/oqs-provider/lib"
export OPENSSL_CONF="/opt/openssl/openssl.cnf"  # Adjusted if needed

SRC="benchmarks/kyber/benchmark_kyber.c"
BIN="binaries/kyber/benchmark_kyber"
OUT="results/kyber/benchmark_kyber.csv"

# Check if gcc is available
if ! command -v gcc &>/dev/null; then
    echo "[!] gcc not found. Please install it."
    exit 1
fi

# Create required directories
mkdir -p "$(dirname "$BIN")"
mkdir -p "$(dirname "$OUT")"

# --- Certificate and Key Generation for Kyber ---
# Note: Typically, KEMs (like Kyber) are used for key exchange rather than signatures,
# and certificate generation using a KEM algorithm is experimental. 
# This block generates a self-signed certificate using OQS_KEM_KYBER_512
# To allow vlagrind to run
CERT="certs/kyber_cert.pem"
KEY="certs/kyber_key.pem"

if [ ! -f "$CERT" ] || [ ! -f "$KEY" ]; then
    echo "[*] Generating Kyber certificate and key..."
    openssl req -x509 -days 365 -new -newkey OQS_KEM_KYBER_512 -nodes \
      -keyout "$KEY" -out "$CERT" -subj "/CN=localhost"
fi

echo "[*] Compiling Kyber benchmark..."
# Use the updated gcc command to compile and link the source file
gcc -o "$BIN" "$SRC" -lssl -lcrypto -ldl

echo "[*] Running Kyber benchmark..."
start_time=$(date +%s)

if ! "$BIN"; then
    echo "[!] Kyber benchmark binary failed to execute."
    exit 1
fi

end_time=$(date +%s)
runtime=$((end_time - start_time))

if [ -f "$OUT" ]; then
    echo "[✓] Kyber results saved to $OUT"
    echo "[⏱] Runtime: ${runtime} seconds"
else
    echo "[!] Kyber benchmark did not produce output file: $OUT"
    exit 1
fi
