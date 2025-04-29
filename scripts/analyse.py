#!/usr/bin/env python3

import os
import re
import pandas as pd
import matplotlib.pyplot as plt

# ----------------------------
# Configuration
# ----------------------------
ALGORITHMS = {
    "Kyber": {
        "csv": "~/PQC_Data/results/kyber/benchmark_kyber.csv",
        "tls": "~/PQC_Data/results/kyber/benchmark_kyber_tls_time.log",
    },
    "Dilithium": {
        "csv": "results/dilithium/benchmark_dilithium.csv",
        "tls": "~/PQC_Data/results/dilithium/benchmark_dilithium_tls_time.log",
    },
    "ECDSA": {
        "csv": "~/PQC_Data/results/classical_ecdsa/benchmark_ecdsa.csv",
        "tls": "~/PQC_Data/results/classical_ecdsa/benchmark_ecdsa_tls_time.log",
    },
    "RSA": {
        "csv": "~/PQC_Data/results/classical_rsa/benchmark_rsa.csv",
        "tls": "~/PQC_Data/results/classical_rsa/benchmark_rsa_tls_time.log",
    },
    "ECDH": {
        "csv": "~/PQC_Data/results/classical_ecdh/benchmark_ecdh.csv",
        "tls": "~/PQC_Data/results/classical_ecdh/benchmark_ecdh_tls_time.log",
    },
}

COLORS = {
    "Kyber": "blue",
    "Dilithium": "green",
    "ECDSA": "orange",
    "RSA": "purple",
    "ECDH": "red"
}

PLOTS_DIR = "results/plots"
os.makedirs(PLOTS_DIR, exist_ok=True)

# ----------------------------
# Benchmark CSV Analysis
# ----------------------------
def load_benchmark_stats(path):
    try:
        df = pd.read_csv(path)
        if "time_ms" not in df.columns:
            print(f"[!] 'time_ms' column missing in {path}")
            return None

        # Heuristic: Convert seconds to ms if values are way too small
        if df["time_ms"].mean() < 1:
            df["time_ms"] *= 1000

        return {
            "avg": df["time_ms"].mean(),
            "std": df["time_ms"].std(),
            "min": df["time_ms"].min(),
            "max": df["time_ms"].max(),
            "count": len(df)
        }
    except Exception as e:
        print(f"[!] Error reading {path}: {e}")
        return None

def plot_keygen_metrics(stats):
    if not stats:
        print("[!] No benchmark data found.")
        return

    sorted_algos = sorted(stats.keys(), key=lambda x: stats[x]["avg"])
    avgs = [stats[a]["avg"] for a in sorted_algos]
    stds = [stats[a]["std"] for a in sorted_algos]
    mins = [stats[a]["min"] for a in sorted_algos]
    maxs = [stats[a]["max"] for a in sorted_algos]

    plt.figure(figsize=(10, 6))
    bars = plt.bar(sorted_algos, avgs, yerr=stds, capsize=6,
                   color=[COLORS.get(a, 'gray') for a in sorted_algos],
                   edgecolor='black')

    plt.ylabel("Key Generation Time (ms)")
    plt.title("EVP Benchmark: Avg Key Generation Time with Std Dev")

    for i, algo in enumerate(sorted_algos):
        plt.text(i, avgs[i] + stds[i] + 1, f"min: {mins[i]:.1f} ms", ha='center', fontsize=8)
        plt.text(i, avgs[i] + stds[i] + 5, f"max: {maxs[i]:.1f} ms", ha='center', fontsize=8)

    plt.tight_layout()
    path = f"{PLOTS_DIR}/benchmark_{algo}.png"
    plt.savefig(path)
    print(f"[✓] Saved benchmark plot: {path}")

# ----------------------------
# TLS Metric Extraction
# ----------------------------
def extract_tls_metric(path, label, cast_type=float, scale=1):
    try:
        with open(path, "r") as f:
            for line in f:
                if label in line:
                    match = re.search(r"([\d.]+)", line)
                    if match:
                        return cast_type(match.group(1)) * scale
    except FileNotFoundError:
        print(f"[!] Missing TLS file: {path}")
    return None

def plot_tls_metric(metric_dict, title, ylabel, filename):
    metric_dict = {k: v for k, v in metric_dict.items() if v is not None}
    if not metric_dict:
        print(f"[!] Skipping {title} – no valid data.")
        return

    plt.figure(figsize=(10, 6))
    bars = plt.bar(metric_dict.keys(), metric_dict.values(),
                   color=[COLORS.get(k, "gray") for k in metric_dict.keys()],
                   edgecolor="black")

    plt.ylabel(ylabel)
    plt.title(title)

    for bar in bars:
        height = bar.get_height()
        plt.text(bar.get_x() + bar.get_width()/2, height + 0.01,
                 f"{height:.2f}", ha='center', va='bottom', fontsize=8)

    plt.tight_layout()
    path = f"{PLOTS_DIR}/{filename}.png"
    plt.savefig(path)
    print(f"[✓] Saved TLS plot: {path}")

# ----------------------------
# Main Orchestration
# ----------------------------
def main():
    # --- Keygen Benchmark Summary ---
    keygen_stats = {}
    for algo, paths in ALGORITHMS.items():
        if os.path.exists(paths["csv"]):
            stats = load_benchmark_stats(paths["csv"])
            if stats:
                keygen_stats[algo] = stats
    plot_keygen_metrics(keygen_stats)

    # --- TLS Metrics Summary ---
    tls_user_time = {}
    tls_mem_usage = {}
    tls_task_clock = {}
    tls_page_faults = {}
    tls_context_switches = {}

    for algo, paths in ALGORITHMS.items():
        log = paths["tls"]
        if not os.path.exists(log):
            continue
        tls_user_time[algo] = extract_tls_metric(log, "User time", float)
        tls_mem_usage[algo] = extract_tls_metric(log, "Maximum resident set size", int, 1/1024)
        tls_task_clock[algo] = extract_tls_metric(log, "Task-clock", float)
        tls_page_faults[algo] = extract_tls_metric(log, "Page-faults", int)
        tls_context_switches[algo] = extract_tls_metric(log, "Context-switches", int)

    plot_tls_metric(tls_user_time, "TLS Handshake Time", "User Time (s)", "tls_user_time")
    plot_tls_metric(tls_mem_usage, "Memory Usage (TLS Handshake)", "Memory (MB)", "tls_memory")
    plot_tls_metric(tls_task_clock, "Task Clock (TLS Handshake)", "Clock Ticks", "tls_task_clock")
    plot_tls_metric(tls_page_faults, "Page Faults (TLS Handshake)", "Page Faults", "tls_page_faults")
    plot_tls_metric(tls_context_switches, "Context Switches (TLS Handshake)", "Count", "tls_context_switches")

if __name__ == "__main__":
    main()
