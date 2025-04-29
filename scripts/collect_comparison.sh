#!/bin/bash
set -euo pipefail

cd "$(dirname "$0")/.." || exit 1

# --- Environment setup ---
export OPENSSL="/opt/openssl/bin/openssl"
export LD_LIBRARY_PATH="/opt/openssl/lib64:$LD_LIBRARY_PATH"
export OPENSSL_MODULES="/opt/openssl/lib64/ossl-modules"
export OPENSSL_CONF="/opt/openssl/openssl.cnf"

CERT_DIR="certs"
RESULTS_DIR="results"
LOGDIR="$(pwd)/logs"

mkdir -p "$CERT_DIR" "$LOGDIR"
mkdir -p "$RESULTS_DIR/classical_ecdsa" "$RESULTS_DIR/dilithium"

# -----------------------------
generate_cert() {
    local algo=$1
    local key=$2
    local cert=$3

    echo "[*] Generating ${algo} certificate..."

    if [[ "$algo" == "dilithium2" || "$algo" == "p256_dilithium2" || "$algo" == "p256_mldsa44" ]]; then
        # Generate a new certificate using OQS provider for Dilithium (post-quantum)
        openssl req -provider oqs -provider default \
            -new -newkey "$algo" -nodes -keyout "$key" -out tmp_req.pem -subj "/CN=localhost"
        
        # Sign the certificate with the post-quantum private key
        openssl x509 -provider oqs -provider default \
            -req -in tmp_req.pem -signkey "$key" -out "$cert" -days 1
        rm -f tmp_req.pem
    elif [[ "$algo" == "ec" ]]; then
        # Handle EC certificates (ECDSA) for classical algorithms
        openssl req -provider default \
            -x509 -newkey ec -pkeyopt ec_paramgen_curve:P-256 -nodes \
            -keyout "$key" -out "$cert" -days 1 -subj "/CN=localhost"
    else
        # Handle other traditional algorithms like RSA
        openssl req -provider oqs -provider default \
            -x509 -newkey "$algo" -nodes \
            -keyout "$key" -out "$cert" -days 1 -subj "/CN=localhost"
    fi
}

wait_for_tls() {
    echo "[*] Waiting for TLS server to become ready on port 4443..."
    for i in {1..10}; do
        if nc -z localhost 4443; then
            echo "[✓] TLS server is listening on port 4443"
            return 0
        fi
        sleep 1
    done
    echo "[!] TLS server did not become available on port 4443"
    return 1
}

run_tls_test() {
    local label=$1
    local cert_file=$2
    local key_file=$3
    local output_dir=$4

    mkdir -p "$output_dir"
    local log_file="$LOGDIR/server_${label}.log"

    echo "[DEBUG] Using certificate file: $cert_file"
    echo "[DEBUG] Using key file: $key_file"
    echo "[*] Starting TLS server (${label})..."

    if [[ "$label" == "ecdsa" ]]; then
        # For classical ECDSA, use only the default provider (no oqs options)
        # and allow multiple connections (-naccept 5) so the server stays up longer.
        openssl s_server -accept 4443 -cert certs/ecdsa_cert.pem -key certs/ecdsa_key.pem -tls1_3 -naccept 5 &\
            > "$log_file" 2>&1 &
    else
        # For non-classical tests, include the oqs provider options.
        openssl s_server -provider oqs -provider default -provider-path /opt/openssl/lib64/ossl-modules \
            -accept 4443 -cert "$cert_file" -key "$key_file" -tls1_3 \
            > "$log_file" 2>&1 &
    fi
    SERVER_PID=$!

    sleep 5  # Increase wait time for server to fully initialize

    # Wait for TLS server to become ready
    if ! wait_for_tls; then
        echo "[!] Server startup failed"
        echo "[!] See server logs for more details: $log_file"
        kill $SERVER_PID 2>/dev/null
        return 1
    fi

    # Try connecting multiple times if necessary
    for attempt in {1..5}; do
        if ps -p $SERVER_PID > /dev/null; then
            echo "[*] Attempting TLS client connection (${label}), attempt ${attempt}..."
            # Use the timeout command (10 seconds in this example) to limit the connection attempt duration.
            timeout 10 openssl s_client -tls1_3 -connect localhost:4443 \
                > "$output_dir/${label}_tls_client.log" \
                2> "$output_dir/${label}_tls_time.log"

            if grep -q "Cipher is" "$output_dir/${label}_tls_client.log"; then
                echo "[✓] TLS client successfully connected (attempt $attempt)"
                break
            else
                echo "[!] Failed attempt $attempt — retrying..."
                sleep 2
            fi
        else
            echo "[!] TLS server process died before client could connect (attempt $attempt)"
            break
        fi
    done


    # Optional: collect performance metrics
    if ps -p $SERVER_PID > /dev/null; then
        echo "[*] Collecting performance metrics..."
        perf stat -e task-clock,instructions,context-switches -o "$output_dir/${label}_tls_perf.log" \
            openssl s_client -tls1_3 -connect localhost:4443 > /dev/null 2>&1 || true
    fi

    kill $SERVER_PID 2>/dev/null || true
    wait $SERVER_PID 2>/dev/null || true
}

# -----------------------------
# MAIN
# Generating ECDSA certificate
generate_cert ec "$CERT_DIR/ecdsa_key.pem" "$CERT_DIR/ecdsa_cert.pem"
run_tls_test "ecdsa" "$CERT_DIR/ecdsa_cert.pem" "$CERT_DIR/ecdsa_key.pem" "$RESULTS_DIR/classical_ecdsa"

# The Dilithium-based certificate generation has been commented out for now.
# Uncomment the following lines when post-quantum certificate support is ready.
#
# generate_cert p256_mldsa44 "$CERT_DIR/dilithium_key.pem" "$CERT_DIR/dilithium_cert.pem"
# run_tls_test "dilithium" "$CERT_DIR/dilithium_cert.pem" "$CERT_DIR/dilithium_key.pem" "$RESULTS_DIR/dilithium"

echo "[✓] TLS comparison complete. Results saved in ${RESULTS_DIR}/"
