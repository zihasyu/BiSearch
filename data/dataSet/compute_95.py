#!/usr/bin/env python3
import os
import csv
import math

DEPTH1_DIR = "./depth1"
DEPTH50_DIR = "./depth50"

DATASETS = ["Cpython", "WEB", "automake", "coreutils", "gcc", "linux", "netty", "react"]

def read_column(filepath, col_name):
    values = []
    with open(filepath, "r") as f:
        reader = csv.DictReader(f)
        for row in reader:
            try:
                values.append(float(row[col_name]))
            except (ValueError, KeyError):
                continue
    return values

def _norm_ppf(p):
    """Rational approximation for inverse normal CDF (Abramowitz & Stegun 26.2.23)"""
    if p <= 0 or p >= 1:
        raise ValueError("p must be in (0, 1)")
    if p < 0.5:
        return -_norm_ppf(1 - p)
    t = math.sqrt(-2 * math.log(1 - p))
    c0, c1, c2 = 2.515517, 0.802853, 0.010328
    d1, d2, d3 = 1.432788, 0.189269, 0.001308
    return t - (c0 + c1 * t + c2 * t**2) / (1 + d1 * t + d2 * t**2 + d3 * t**3)

def _t_ppf(p, df):
    """Approximation of inverse t-distribution CDF (Cornish-Fisher expansion)"""
    z = _norm_ppf(p)
    z2 = z * z
    z3 = z2 * z
    z5 = z3 * z2
    z7 = z5 * z2
    z9 = z7 * z2
    df2 = df * df
    df3 = df2 * df
    df4 = df3 * df
    t = z
    t += (z3 + z) / (4 * df)
    t += (5 * z5 + 16 * z3 + 3 * z) / (96 * df2)
    t += (3 * z7 + 19 * z5 + 17 * z3 - 15 * z) / (384 * df3)
    t += (79 * z9 + 776 * z7 + 1482 * z5 - 1920 * z3 - 945 * z) / (92160 * df4)
    return t

def mean_ci(data, confidence=0.95):
    n = len(data)
    if n == 0:
        return 0, 0
    mean = sum(data) / n
    if n < 2:
        return mean, 0
    variance = sum((x - mean) ** 2 for x in data) / (n - 1)
    se = math.sqrt(variance / n)
    ci_half = se * _t_ppf((1 + confidence) / 2, n - 1)
    return mean, ci_half

print(f"{'Dataset':<15} | {'Metric':<25} | {'Depth1 Mean':>12} {'±CI':>10} | {'Depth50 Mean':>12} {'±CI':>10}")
print("-" * 105)

for ds in DATASETS:
    for metric_file, metric_col, metric_label in [
        ("detailed_log.csv", "Throughput_MB/s", "Commit Throughput"),
        ("restore_log.csv", "RestoreThroughput_MB/s", "Restore Throughput"),
    ]:
        f1 = os.path.join(DEPTH1_DIR, f"{ds}_{metric_file}")
        f50 = os.path.join(DEPTH50_DIR, f"{ds}_{metric_file}")

        d1_values = read_column(f1, metric_col) if os.path.exists(f1) else []
        d50_values = read_column(f50, metric_col) if os.path.exists(f50) else []

        m1, ci1 = mean_ci(d1_values)
        m50, ci50 = mean_ci(d50_values)

        print(f"{ds:<15} | {metric_label:<25} | {m1:>12.2f} {ci1:>10.2f} | {m50:>12.2f} {ci50:>10.2f}")