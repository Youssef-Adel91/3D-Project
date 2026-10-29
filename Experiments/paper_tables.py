#!/usr/bin/env python3
"""
paper_tables.py
Extracts and formats all tables for the paper strictly from CSV results.
Outputs tables as plain text and compares measured values with cited paper values.
"""

import os
import sys
import pandas as pd
import numpy as np

def find_file(filename):
    search_paths = [
        os.path.join("Experiments", "Results", filename),
        os.path.join("Results", filename),
        os.path.join("Experiments", filename),
        filename
    ]
    for p in search_paths:
        if os.path.exists(p):
            return p
    raise FileNotFoundError(f"Could not find {filename} in search paths: {search_paths}")

def main():
    lines = []
    def log(msg=""):
        lines.append(msg)
        print(msg)

    log("=" * 80)
    log("TRACEABILITY OF ALL PAPER NUMBERS TO EXPERIMENT CSVs")
    log("=" * 80)
    log()

    # -------------------------------------------------------------
    # 1. Experiment 1: Deformation Quality (e1_quality.csv)
    # -------------------------------------------------------------
    log("1. EXPERIMENT 1: DEFORMATION QUALITY (dq.gltf bar)")
    log("-" * 80)
    e1_path = find_file("e1_quality.csv")
    e1_df = pd.read_csv(e1_path)
    log(f"Source file: {e1_path}")
    log()
    log(f"{'Motion':<8} {'Angle':<8} {'LBS Vol':<12} {'LBS MinThick':<14} {'DQS Vol':<12} {'DQS MinThick':<14}")
    log("-" * 72)

    e1_rows = [
        ("Twist", 90.0),
        ("Twist", 120.0),
        ("Twist", 180.0),
        ("Bend", 90.0),
        ("Bend", 120.0),
    ]

    for motion, deg in e1_rows:
        lbs = e1_df[(e1_df['motion'] == motion) & (e1_df['angle_deg'] == deg) & (e1_df['method'] == 'LBS')].iloc[0]
        dqs = e1_df[(e1_df['motion'] == motion) & (e1_df['angle_deg'] == deg) & (e1_df['method'] == 'DQS')].iloc[0]
        log(f"{motion:<8} {int(deg):<8} {lbs['volume_ratio']:<12.6f} {lbs['min_thickness_ratio']:<14.6f} {dqs['volume_ratio']:<12.6f} {dqs['min_thickness_ratio']:<14.6f}")

    log()

    # -------------------------------------------------------------
    # 2. Experiment 1b: Sensitivity to Weight Smoothness (e1b_quality.csv)
    # -------------------------------------------------------------
    log("2. EXPERIMENT 1b: SENSITIVITY TO WEIGHT SMOOTHNESS W (Twist 180 deg)")
    log("-" * 80)
    e1b_path = find_file("e1b_quality.csv")
    e1b_df = pd.read_csv(e1b_path)
    log(f"Source file: {e1b_path}")
    log()
    log(f"{'W':<8} {'LBS Vol':<12} {'Analytical (1 - W/15)':<24} {'Diff (LBS - Form)':<20} {'DQS Vol':<12} {'DQS MinThick':<14}")
    log("-" * 92)

    t180 = e1b_df[(e1b_df['motion'] == 'twist') & (e1b_df['angle_deg'] == 180.0)]
    w_vals = sorted(t180['W'].unique())

    for w in w_vals:
        lbs = t180[(t180['W'] == w) & (t180['method'] == 'LBS')].iloc[0]
        dqs = t180[(t180['W'] == w) & (t180['method'] == 'DQS')].iloc[0]
        formula_vol = 1.0 - (w / 15.0)
        diff = lbs['volume_ratio'] - formula_vol
        log(f"{w:<8.2f} {lbs['volume_ratio']:<12.6f} {formula_vol:<24.6f} {diff:<+20.6f} {dqs['volume_ratio']:<12.6f} {dqs['min_thickness']:<14.6f}")

    log()

    # -------------------------------------------------------------
    # 3. Experiment 2: GPU Performance (e2_gpu.csv)
    # -------------------------------------------------------------
    log("3. EXPERIMENT 2: GPU TIMINGS (60 Bones, 4 Influences)")
    log("-" * 80)
    e2_path = find_file("e2_gpu.csv")
    e2_df = pd.read_csv(e2_path)
    log(f"Source file: {e2_path}")
    log()

    # Table 3a: Vertex Count Scaling (median [min-max over 5 runs])
    log("Table 3a: GPU Time (ms) by Vertex Count at 60 Bones, 4 Influences:")
    log(f"{'Variant':<12} {'9,996 Vertices':<24} {'99,852 Vertices':<24} {'999,996 Vertices':<24}")
    log("-" * 86)

    variants = ['LBS-naive', 'LBS-pre', 'DQS-naive', 'DQS-pre']
    v_counts = [9996, 99852, 999996]

    e2_table_data = {}
    for var in variants:
        row_str = f"{var:<12}"
        for vc in v_counts:
            sub = e2_df[(e2_df['bones'] == 60) & (e2_df['influences'] == 4) & (e2_df['vertices'] == vc) & (e2_df['variant'] == var)]
            # 5 runs
            med = sub['gpu_ms_median'].median()
            mn = sub['gpu_ms_median'].min()
            mx = sub['gpu_ms_median'].max()
            e2_table_data[(var, vc)] = med
            cell = f"{med:.3f} [{mn:.3f}-{mx:.3f}]"
            row_str += f" {cell:<24}"
        log(row_str)

    log()

    # Table 3b: Bone-Count Scaling at 99,852 Vertices, 4 Influences
    log("Table 3b: Bone-Count Scaling at 99,852 Vertices, 4 Influences (median ms across 5 runs):")
    bone_counts = [20, 43, 60, 120]
    header = f"{'Variant':<12}" + "".join([f"{b} bones".rjust(12) for b in bone_counts])
    log(header)
    log("-" * len(header))

    for var in variants:
        row_str = f"{var:<12}"
        for b in bone_counts:
            sub = e2_df[(e2_df['vertices'] == 99852) & (e2_df['influences'] == 4) & (e2_df['bones'] == b) & (e2_df['variant'] == var)]
            med = sub['gpu_ms_median'].median()
            row_str += f"{med:12.4f}"
        log(row_str)

    log()

    # -------------------------------------------------------------
    # 4. CPU Palette Construction & Upload Times (e2_gpu.csv)
    # -------------------------------------------------------------
    log("4. CPU PALETTE & UPLOAD TIMES (Microseconds)")
    log("-" * 80)
    log(f"{'Variant':<12} {'Bones':<8} {'CPU Palette (us)':<20} {'GPU Upload (us)':<20}")
    log("-" * 62)

    cpu_palette_data = {}
    for b in [20, 60, 120]:
        for var in ['LBS-pre', 'DQS-pre']:
            sub = e2_df[(e2_df['vertices'] == 99852) & (e2_df['influences'] == 4) & (e2_df['bones'] == b) & (e2_df['variant'] == var)]
            pal_us = sub['cpu_palette_ms'].median() * 1000.0
            up_us = sub['upload_ms'].median() * 1000.0
            cpu_palette_data[(var, b)] = pal_us
            log(f"{var:<12} {b:<8} {pal_us:<20.2f} {up_us:<20.2f}")

    log()

    # -------------------------------------------------------------
    # 5. Experiment 3: Upload Paths Scaling (e3_path.csv)
    # -------------------------------------------------------------
    log("5. EXPERIMENT 3: UPLOAD PATHS (Uniform vs UBO vs TBO)")
    log("-" * 80)
    e3_path = find_file("e3_path.csv")
    e3_df = pd.read_csv(e3_path)
    log(f"Source file: {e3_path}")
    log()

    e3_bones = [60, 120, 256, 512]
    paths = ['uniform', 'ubo', 'tbo']

    log(f"{'Variant':<10} {'Bones':<8} {'Uniform (ms)':<16} {'UBO (ms)':<16} {'TBO (ms)':<16}")
    log("-" * 68)

    e3_table_data = {}
    for var in ['LBS-pre', 'DQS-pre']:
        for b in e3_bones:
            row_str = f"{var:<10} {b:<8}"
            for p in paths:
                sub = e3_df[(e3_df['variant'] == var) & (e3_df['bones'] == b) & (e3_df['path'] == p)]
                if len(sub) == 0:
                    val_str = "N/A"
                else:
                    status = sub['status'].iloc[0]
                    if status == 'exceeds_limit':
                        val_str = "limit"
                    else:
                        val = sub['gpu_ms_median'].median()
                        val_str = f"{val:.4f}"
                        e3_table_data[(var, b, p)] = val
                row_str += f" {val_str:<15}"
            log(row_str)

    log()

    # -------------------------------------------------------------
    # 6. Budget Table (Frame Time Budget at 60 Hz = 16.667 ms)
    # -------------------------------------------------------------
    log("6. 60 FPS FRAME BUDGET TABLE (16.667 ms budget)")
    log("-" * 80)
    log(f"{'Configuration':<38} {'Time (ms)':<12} {'% of 16.667 ms Budget':<24}")
    log("-" * 76)

    budget_items = [
        ("LBS-naive at 99,852 verts (60 bones)", e2_table_data[('LBS-naive', 99852)]),
        ("LBS-pre at 99,852 verts (60 bones)",   e2_table_data[('LBS-pre', 99852)]),
        ("DQS-naive at 99,852 verts (60 bones)", e2_table_data[('DQS-naive', 99852)]),
        ("DQS-pre at 99,852 verts (60 bones)",   e2_table_data[('DQS-pre', 99852)]),
        ("LBS-naive at 999,996 verts (60 bones)", e2_table_data[('LBS-naive', 999996)]),
        ("LBS-pre at 999,996 verts (60 bones)",   e2_table_data[('LBS-pre', 999996)]),
        ("DQS-naive at 999,996 verts (60 bones)", e2_table_data[('DQS-naive', 999996)]),
        ("DQS-pre at 999,996 verts (60 bones)",   e2_table_data[('DQS-pre', 999996)]),
    ]

    for label, t in budget_items:
        pct = (t / 16.667) * 100.0
        log(f"{label:<38} {t:<12.3f} {pct:<24.2f}%")

    log()

    # -------------------------------------------------------------
    # 7. Discrepancy & Paper Value Comparison
    # -------------------------------------------------------------
    log("7. COMPARISON WITH CITED PAPER VALUES")
    log("-" * 80)
    log(f"{'Metric / Claim':<42} {'Cited in Paper':<16} {'CSV Value':<14} {'Difference':<12} {'Diff > Tol?':<12}")
    log("-" * 96)

    # Targets to compare:
    comparisons = [
        ("E1 Twist 180 LBS Volume Ratio", 0.838, e1_df[(e1_df['motion']=='Twist')&(e1_df['angle_deg']==180.0)&(e1_df['method']=='LBS')]['volume_ratio'].iloc[0], 0.0005, "vol"),
        ("E1 Twist 180 DQS Volume Ratio", 0.994, e1_df[(e1_df['motion']=='Twist')&(e1_df['angle_deg']==180.0)&(e1_df['method']=='DQS')]['volume_ratio'].iloc[0], 0.0005, "vol"),
        ("E2 99,852 verts LBS-naive (ms)", 0.365, e2_table_data[('LBS-naive', 99852)], 0.002, "time"),
        ("E2 99,852 verts LBS-pre (ms)",   0.102, e2_table_data[('LBS-pre', 99852)],   0.002, "time"),
        ("E2 99,852 verts DQS-naive (ms)", 0.132, e2_table_data[('DQS-naive', 99852)], 0.002, "time"),
        ("E2 99,852 verts DQS-pre (ms)",   0.090, e2_table_data[('DQS-pre', 99852)],   0.002, "time"),
        ("E2 999,996 verts LBS-naive (ms)", 2.158, e2_table_data[('LBS-naive', 999996)], 0.002, "time"),
        ("E2 999,996 verts LBS-pre (ms)",   0.932, e2_table_data[('LBS-pre', 999996)],   0.002, "time"),
        ("E2 999,996 verts DQS-naive (ms)", 1.060, e2_table_data[('DQS-naive', 999996)], 0.002, "time"),
        ("E2 999,996 verts DQS-pre (ms)",   0.913, e2_table_data[('DQS-pre', 999996)],   0.002, "time"),
        ("E3 LBS-pre 512 bones UBO (ms)",   0.684, e3_table_data.get(('LBS-pre', 512, 'ubo'), np.nan), 0.002, "time"),
        ("E3 LBS-pre 512 bones TBO (ms)",   0.080, e3_table_data.get(('LBS-pre', 512, 'tbo'), np.nan), 0.002, "time"),
        ("CPU Palette 60 bones LBS-pre (us)", 6.2, cpu_palette_data.get(('LBS-pre', 60), np.nan), 0.5, "us"),
        ("CPU Palette 60 bones DQS-pre (us)", 8.4, cpu_palette_data.get(('DQS-pre', 60), np.nan), 0.5, "us"),
    ]

    discrepancies = []
    for label, cited, measured, tol, kind in comparisons:
        diff = measured - cited
        is_exceeded = abs(diff) > tol
        flag = "EXCEEDED" if is_exceeded else "MATCH"
        if is_exceeded:
            discrepancies.append((label, cited, measured, diff, tol))
        if kind == "vol":
            log(f"{label:<42} {cited:<16.3f} {measured:<14.6f} {diff:<+12.6f} {flag:<12}")
        elif kind == "us":
            log(f"{label:<42} {cited:<16.1f} {measured:<14.2f} {diff:<+12.2f} {flag:<12}")
        else:
            log(f"{label:<42} {cited:<16.3f} {measured:<14.4f} {diff:<+12.4f} {flag:<12}")

    log()
    log(f"Summary: {len(discrepancies)} values had differences exceeding the comparison tolerance:")
    if not discrepancies:
        log("  None. All cited values match CSV measurements within tolerance.")
    else:
        for d in discrepancies:
            log(f"  * {d[0]}: cited {d[1]}, measured {d[2]:.4f}, diff {d[3]:+.4f} (tol {d[4]})")

    log("=" * 80)

    # Write output file
    out_path = "paper_tables_output.txt"
    with open(out_path, "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")
    print(f"\nWritten complete output to {out_path}")

    # Also save in root if run from subdirectory
    if os.path.exists("..") and os.path.isdir(".."):
        try:
            with open(os.path.join("..", out_path), "w", encoding="utf-8") as f:
                f.write("\n".join(lines) + "\n")
            print(f"Also written to ../{out_path}")
        except Exception:
            pass

if __name__ == "__main__":
    main()
