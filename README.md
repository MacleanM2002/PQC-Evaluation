<h1>PQC-TLS Benchmarking and Analysis Suite</h1>

This repository provides a full framework for benchmarking Post-Quantum Cryptography (PQC) algorithms (such as Kyber and Dilithium) and classical algorithms (RSA, ECDSA, ECDH) under TLS 1.3 connections, including:

Benchmarking Key Generation, Signing, and Verification times

TLS 1.3 server-client handshake testing

Automated collection, analysis, and plotting of results

Repository Structure
``` bash
PQC_Data/
├── benchmarks/           # C benchmark sources for Kyber, Dilithium, RSA, ECDH, ECDSA
├── binaries/             # Compiled benchmarking binaries
├── certs/                # Generated TLS certificates and private keys
├── results/              # Benchmark results, TLS logs, plots
├── scripts/              # Automation scripts (shell + Python)
├── Sample_Data/          # Sample files for signing/verification
├── logs/                 # Server/client log outputs
```
<h3>Prerequisites</h3>

* Linux (Ubuntu/Debian recommended)

* GCC (for compiling C benchmarks)

* OpenSSL 3.0.x with OQS provider integration

* Python 3.x with matplotlib, pandas

* valgrind (optional for memory leak checking)

Ensure your environment is properly configured:
```
export OPENSSL=/opt/openssl/bin/openssl
export LD_LIBRARY_PATH=/opt/openssl/lib64:$LD_LIBRARY_PATH
export OPENSSL_MODULES=/opt/oqs-provider/lib
export OPENSSL_CONF=/opt/openssl/openssl.cnf
```
<h3>Installation and Setup</h3>

Clone the repository:
```
git clone https://github.com/YOUR_USERNAME/PQC-TLS-Benchmark.git
cd PQC-TLS-Benchmark
```
Build benchmarking binaries:
```
cd benchmarks/
# Example: build dilithium benchmark
gcc dilithium/benchmark_dilithium.c -o ../binaries/dilithium/benchmark_dilithium -lcrypto -lrt
# Repeat similarly for kyber, RSA, ECDSA, ECDH
```
Make scripts executable:
```
chmod +x scripts/*.sh
```
Prepare folders:
```
mkdir -p results certs logs binaries scripts
```
Check OpenSSL installation:
```
$OPENSSL version
```
Ensure it lists the OQS provider if correctly installed.

<h3>Running Benchmarks and TLS Tests</h3>

Simply run:
```
./scripts/collect_all.sh
```
This will:

* Generate keys/certificates for RSA, ECDSA, Kyber, and Dilithium

* Launch TLS server/client handshakes

* Run individual cryptographic benchmarks

* Save results and plots under the results/ directory

Example Output:
```
[*] Benchmarking RSA...
[*] Running TLS Comparison...
[✓] Benchmark completed. Results saved in results/plots/
```
<h3>Analyzing Results</h3>

To generate plots and detailed statistics:
```
python3 scripts/analyse.py
python3 scripts/TLS_Comparison.py
```
This will create visualizations in results/plots/ for easy comparison.

<h3>Troubleshooting</h3>

* If TLS handshake fails, ensure server is properly listening.

* Confirm certificates are correctly generated under certs/.

* Ensure OpenSSL with OQS is properly loaded (check OPENSSL_MODULES environment).

Common Errors:

* BIO_connect:Connection refused -> TLS server is not up yet.

* Error allocating keygen context -> OQS provider not correctly loaded.

<h3>Contributions</h3>

Contributions are welcome! Feel free to open issues or pull requests to improve:

* Support for more PQC algorithms

* Better error handling

* More plotting/analysis tools

<h3>License</h3>

This project is licensed under the MIT License.

<h3>Acknowledgements</h3>

* [Open Quantum Safe Project](https://openquantumsafe.org/)

* [OpenSSL Project](https://www.openssl.org/)
