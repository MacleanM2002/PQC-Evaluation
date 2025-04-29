#!/bin/bash
set -euo pipefail

# Change to the repository root (assumed to be one level up from the script location)
cd "$(dirname "$0")/.." || exit 1
start_time=$(date +%s)

# --- Environment setup ---
export OPENSSL="/opt/openssl/bin/openssl"
export LD_LIBRARY_PATH="/opt/openssl/lib64:$LD_LIBRARY_PATH"
export OPENSSL_CONF="/opt/openssl/openssl.cnf"  # Use our custom OpenSSL config

echo "[*] Starting full PQC + classical benchmark & TLS collection..."

# --- Check OpenSSL installation ---
if ! $OPENSSL version > /dev/null 2>&1; then
    echo "[!] OpenSSL binary not found or not executable at $OPENSSL"
    exit 1
fi

# --- Create required directories ---
mkdir -p ~/PQC_Data/results/{kyber,dilithium,classical_rsa,classical_ecdsa,classical_ecdh,plots}
mkdir -p certs logs binaries scripts

LOGDIR="$(pwd)/logs"

# --- Ensure all scripts and binaries are executable ---
echo "[*] Ensuring all scripts and binaries are executable..."
chmod +x ~/PQC_Data/scripts/*.sh
chmod +x ~/PQC_Data/binaries/*/benchmark_* || true
chmod +x ./scripts/TLS_Comparison.py
chmod +x ./scripts/analyse.py


# --- Show available OpenSSL algorithms for errors when needed---
#echo "[*] Available public key algorithms:"
#$OPENSSL list -public-key-algorithms

# --- Run Benchmarks ---
echo "[*] Running Kyber benchmark..."
./scripts/collect_kyber.sh

echo "[*] Running Dilithium benchmark..."
./scripts/collect_dilithium.sh

echo "[*] Running ECDSA benchmark..."
./scripts/collect_ecdsa.sh

echo "[*] Running ECDH benchmark..."
./scripts/collect_ecdh.sh

echo "[*] Running RSA benchmark..."
./scripts/collect_rsa.sh

# --- TLS Comparison Test ---
#echo "[*] Running TLS Comparison (PQC + Classical)..."
#./scripts/collect_comparison.sh

# --- Optional: Valgrind Memory Checks ---
mkdir -p results/kyber results/dilithium

if ! command -v valgrind &>/dev/null; then
    echo "[*] Valgrind is not installed; skipping memory checks."
fi

#Python3 analyse.py and TLS_Comparison.py
echo "[*]Comparing Results"
./scripts/TLS_Comparison.py

echo "[*]Analysing Results"
./scripts/analyse.py

end_time=$(date +%s)
echo "[*] Benchmark and TLS collection completed in $(($end_time - $start_time)) seconds."
