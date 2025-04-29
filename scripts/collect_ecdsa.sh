#!/bin/bash
set -euo pipefail

echo "[*] Collecting ECDSA benchmark..."

cd "$(dirname "$0")/.." || exit 1

# --- Environment setup ---
export OPENSSL="/opt/openssl/bin/openssl"
export LD_LIBRARY_PATH="/opt/openssl/lib64:$LD_LIBRARY_PATH"
export OPENSSL_MODULES="/opt/oqs-provider/lib"
export OPENSSL_CONF="/opt/openssl/openssl.cnf"

SRC="benchmarks/ECDSA/benchmark_ecdsa.c"
BIN="binaries/classical_ecdsa/benchmark_ecdsa"
OUT="results/classical_ecdsa/benchmark_ecdsa.csv"

# Check for gcc
if ! command -v gcc &>/dev/null; then
    echo "[!] gcc not found. Please install it first."
    exit 1
fi

# Ensure required directories exist
mkdir -p "$(dirname "$BIN")"
mkdir -p "$(dirname "$OUT")"

echo "[*] Compiling ECDSA benchmark..."
gcc "$SRC" -o "$BIN" -lcrypto -lrt

echo "[*] Running ECDSA benchmark..."
start_time=$(date +%s)

if ! "$BIN"; then
    echo "[!] ECDSA benchmark binary failed to execute."
    exit 1
fi

end_time=$(date +%s)
runtime=$((end_time - start_time))

if [ -f "$OUT" ]; then
    echo "[✓] ECDSA results saved to $OUT"
    echo "[⏱] Runtime: ${runtime} seconds"
else
    echo "[!] ECDSA benchmark ran but did not produce output file: $OUT"
    exit 1
fi
