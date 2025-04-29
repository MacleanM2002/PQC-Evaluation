#!/usr/bin/env python3

import matplotlib.pyplot as plt
import os
import re
import pandas as pd

# ----------------------------
# File paths for TLS logs
# ----------------------------
ALGORITHMS = {
    "Kyber": "results/kyber/benchmark_kyber_tls_time.log",
    "Dilithium": "results/dilithium/benchmark_dilithium_tls_time.log",
    "ECDSA": "results/classical_ecdsa/benchmark_ecdsa_tls_time.log",
    "RSA": "results/classical_rsa/benchmark_rsa_tls_time.log",
    "ECDH": "results/classical_ecdh/benchmark_ecdh_tls_time.log"
}

COLORS = {
    "Kyber": "blue",
    "Dilithium": "green",
    "ECDSA": "orange",
    "RSA": "purple",
    "ECDH": "red"
}

# ----------------------------
# Extract specific metric from file
# ----------------------------
def extract_metric(path, label, cast_type=float, scale=1):
    try:
        with open(path, "r") as f:
            for line in f:
                if label in line:
                    value = re.search(r"([\d.]+)", line)
                    if value:
                        raw_val = cast_type(value.group(1))
                        return raw_val * scale
    except FileNotFoundError:
        print(f"[!] Missing file: {path}")
    return None

# ----------------------------
# Plotting utility
# ----------------------------
def plot_metric(title, ylabel, data, filename):
    data = {k: v for k, v in data.items() if v is not None and pd.notna(v)}
    if not data:
        print(f"[!] Skipping {title} – no valid data.")
        return

    os.makedirs("results/plots", exist_ok=True)
    plt.figure(figsize=(10, 5))

    bars = plt.bar(data.keys(), data.values(),
                   color=[COLORS.get(k, "gray") for k in data.keys()],
                   edgecolor='black')

    for bar in bars:
        height = bar.get_height()
        if pd.notna(height) and height != float('inf'):
            plt.text(bar.get_x() + bar.get_width() / 2,
                     height + 0.01,
                     f"{height:.2f}",
                     ha='center', va='bottom', fontsize=8)

    plt.ylabel(ylabel)
    plt.title(title)
    plt.tight_layout(rect=[0, 0, 1, 0.95])

    output_path = f"results/plots/{filename}.png"
    plt.savefig(output_path)
    print(f"[✓] Saved TLS plot: {output_path}")

# ----------------------------
# Main collection & plotting
# ----------------------------
def main():
    user_times = {}
    mem_usages = {}
    task_clocks = {}
    page_faults = {}
    context_switches = {}

    for algo, path in ALGORITHMS.items():
        os.makedirs(os.path.dirname(path), exist_ok=True)

        user = extract_metric(path, "User time", float)
        mem = extract_metric(path, "Maximum resident set size", int, 1/1024)  # KB → MB
        clock = extract_metric(path, "Task-clock", float)
        faults = extract_metric(path, "Page-faults", int)
        switches = extract_metric(path, "Context-switches", int)

        # Optional: Convert seconds to ms if user time is suspiciously low
        if user is not None and user < 0.001:
            print(f"[i] Converting {algo} user time from s → ms")
            user *= 1000

        if user is not None: user_times[algo] = user
        if mem is not None: mem_usages[algo] = mem
        if clock is not None: task_clocks[algo] = clock
        if faults is not None: page_faults[algo] = faults
        if switches is not None: context_switches[algo] = switches

    plot_metric("TLS Handshake Time", "User Time (s)", user_times, "tls_user_time")
    plot_metric("Memory Usage (TLS Handshake)", "Memory (MB)", mem_usages, "tls_memory")
    plot_metric("Task Clock (TLS Handshake)", "Clock Ticks", task_clocks, "tls_task_clock")
    plot_metric("Page Faults (TLS Handshake)", "Page Faults", page_faults, "tls_page_faults")
    plot_metric("Context Switches (TLS Handshake)", "Count", context_switches, "tls_context_switches")

if __name__ == "__main__":
    main()
