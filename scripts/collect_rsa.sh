#!/bin/bash
set -euo pipefail

echo "[*] Collecting RSA benchmark..."

cd "$(dirname "$0")/.." || exit 1

# --- Environment setup ---
export OPENSSL="/opt/openssl/bin/openssl"
export LD_LIBRARY_PATH="/opt/openssl/lib64:$LD_LIBRARY_PATH"
export OPENSSL_MODULES="/opt/oqs-provider/lib"
export OPENSSL_CONF="/opt/openssl/openssl.cnf"

SRC="benchmarks/RSA/benchmark_rsa.c"
BIN="binaries/classical_rsa/benchmark_rsa"
OUT="results/classical_rsa/benchmark_rsa.csv"

# Check if gcc is available
if ! command -v gcc &>/dev/null; then
    echo "[!] gcc not found. Please install it first."
    exit 1
fi

# Create necessary directories
mkdir -p "$(dirname "$BIN")"
mkdir -p "$(dirname "$OUT")"

echo "[*] Compiling RSA benchmark..."
gcc "$SRC" -o "$BIN" -lcrypto -lrt

echo "[*] Running RSA benchmark..."
start_time=$(date +%s)

if ! "$BIN"; then
    echo "[!] RSA benchmark binary failed to execute."
    exit 1
fi

end_time=$(date +%s)
runtime=$((end_time - start_time))

if [ -f "$OUT" ]; then
    echo "[✓] RSA results saved to $OUT"
    echo "[⏱] Runtime: ${runtime} seconds"
else
    echo "[!] RSA benchmark did not produce output file: $OUT"
    exit 1
fi
