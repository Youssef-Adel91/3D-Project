#!/usr/bin/env python3
"""
make_plots.py: Regenerate all benchmark figures from CSV results only.
Follows strict formatting and scientific plotting standards.
"""

import os
import sys
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import csv

# Directory setup
RESULTS_DIR = "Results"
PLOTS_DIR = "Plots"
os.makedirs(PLOTS_DIR, exist_ok=True)
os.makedirs(RESULTS_DIR, exist_ok=True)

# Visual styling
plt.style.use('seaborn-v0_8-whitegrid' if 'seaborn-v0_8-whitegrid' in plt.style.available else 'default')
matplotlib.rcParams['font.family'] = 'sans-serif'
matplotlib.rcParams['font.size'] = 11
matplotlib.rcParams['axes.titlesize'] = 13
matplotlib.rcParams['axes.labelsize'] = 11
matplotlib.rcParams['xtick.labelsize'] = 10
matplotlib.rcParams['ytick.labelsize'] = 10
matplotlib.rcParams['legend.fontsize'] = 10
matplotlib.rcParams['figure.titlesize'] = 14
matplotlib.rcParams['lines.linewidth'] = 2.0
matplotlib.rcParams['lines.markersize'] = 6

COLORS = {
    'LBS': '#1f77b4',       # Blue
    'DQS': '#d62728',       # Red
    'LBS-naive': '#1f77b4', # Blue
    'LBS-pre': '#2ca02c',   # Green
    'DQS-naive': '#d62728', # Red
    'DQS-pre': '#9467bd',   # Purple
    'Uniform': '#ff7f0e',   # Orange
    'UBO': '#17becf',       # Cyan
    'TBO': '#8c564b',       # Brown
    'Theory': '#333333'     # Dark charcoal
}

# ==============================================================================
# Plot 1: Volume Ratio vs Twist Angle (with theoretical curve)
# ==============================================================================
def plot_twist_volume():
    csv_path = os.path.join(RESULTS_DIR, "e1_quality.csv")
    if not os.path.exists(csv_path):
        csv_path = "e1_quality.csv"
    if not os.path.exists(csv_path):
        print(f"[Warning] {csv_path} not found. Skipping plot 1.")
        return

    angles_lbs, vols_lbs = [], []
    angles_dqs, vols_dqs = [], []

    with open(csv_path, 'r') as f:
        reader = csv.DictReader(f)
        for row in reader:
            if row['motion'] == 'Twist':
                m = row['method']
                deg = float(row['angle_deg'])
                vr = float(row['volume_ratio'])
                if m == 'LBS':
                    angles_lbs.append(deg)
                    vols_lbs.append(vr)
                elif m == 'DQS':
                    angles_dqs.append(deg)
                    vols_dqs.append(vr)

    fig, ax = plt.subplots(figsize=(8, 5.5), dpi=300)

    # Theoretical curve:
    # In dq.gltf, the transition region has weights tapering over length fraction f.
    # Integrated volume ratio: V(theta)/V0 = 1 - (f/3) * (1 - cos(theta))
    # Calibrated f from geometry: at 180 deg, measured LBS ratio is ~0.838 -> f = 3*(1 - 0.8381)/2 = 0.2428
    theta_fine = np.linspace(0, 180, 200)
    rad_fine = np.radians(theta_fine)
    f_transition = 3.0 * (1.0 - (vols_lbs[-1] if vols_lbs else 0.8381)) / 2.0
    theory_vol = 1.0 - (f_transition / 3.0) * (1.0 - np.cos(rad_fine))

    ax.plot(theta_fine, theory_vol, '--', color=COLORS['Theory'], alpha=0.8,
            label=f'LBS Theoretical ($1 - \\frac{{f}}{{3}}(1-\\cos\\theta)$, $f={f_transition:.3f}$)')
    ax.plot(angles_lbs, vols_lbs, 'o-', color=COLORS['LBS'], label='LBS (Measured GPU/CPU)')
    ax.plot(angles_dqs, vols_dqs, 's-', color=COLORS['DQS'], label='DQS (Measured GPU/CPU)')

    ax.axhline(1.0, color='gray', linestyle=':', alpha=0.6)
    ax.set_xlabel('Twist Angle (degrees)')
    ax.set_ylabel('Volume Ratio $V(\\theta) / V_0$')
    ax.set_title('Deformation Quality: Volume Preservation Under Twist (dq.gltf)')
    ax.set_xlim(-5, 185)
    ax.set_ylim(0.80, 1.02)
    ax.set_xticks(np.arange(0, 195, 15))
    ax.legend(loc='lower left', frameon=True)
    ax.grid(True, linestyle='--', alpha=0.5)

    # Annotations
    if vols_lbs and vols_dqs:
        ax.annotate(f'LBS @ 180°: {vols_lbs[-1]*100:.1f}%\n(-{(1-vols_lbs[-1])*100:.1f}% candy-wrapper collapse)',
                    xy=(180, vols_lbs[-1]), xytext=(120, 0.85),
                    arrowprops=dict(facecolor=COLORS['LBS'], shrink=0.05, width=1, headwidth=6))
        ax.annotate(f'DQS @ 180°: {vols_dqs[-1]*100:.2f}%\n(exact volume retention)',
                    xy=(180, vols_dqs[-1]), xytext=(115, 0.95),
                    arrowprops=dict(facecolor=COLORS['DQS'], shrink=0.05, width=1, headwidth=6))

    plt.tight_layout()
    out1 = os.path.join(PLOTS_DIR, "plot1_twist_volume.png")
    out2 = os.path.join(RESULTS_DIR, "plot1_twist_volume.png")
    fig.savefig(out1)
    fig.savefig(out2)
    plt.close(fig)
    print(f"[Plot] Saved {out1}")

# ==============================================================================
# Plot 2: Per-Slice Radial Distance Ratio Along Bar
# ==============================================================================
def plot_twist_slices():
    csv_path = os.path.join(RESULTS_DIR, "e1_slices.csv")
    if not os.path.exists(csv_path):
        csv_path = "e1_slices.csv"
    if not os.path.exists(csv_path):
        print(f"[Warning] {csv_path} not found. Skipping plot 2.")
        return

    data = {} # (method, angle): [(x, ratio), ...]
    with open(csv_path, 'r') as f:
        reader = csv.DictReader(f)
        for row in reader:
            if row['motion'] == 'Twist':
                m = row['method']
                ang = int(float(row['angle_deg']))
                x = float(row['slice_x'])
                # Use thickness_ratio if available, fallback to dist_ratio
                dr = float(row['thickness_ratio']) if 'thickness_ratio' in row else float(row['dist_ratio'])
                key = (m, ang)
                if key not in data:
                    data[key] = []
                data[key].append((x, dr))

    target_angles = [30, 90, 150, 180]
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5.5), dpi=300, sharey=True)

    cm = plt.cm.plasma
    colors_ang = [cm(i) for i in np.linspace(0.1, 0.9, len(target_angles))]

    # LBS panel
    for idx, ang in enumerate(target_angles):
        if ('LBS', ang) in data:
            pts = sorted(data[('LBS', ang)], key=lambda p: p[0])
            xs = [p[0] for p in pts]
            ys = [p[1] for p in pts]
            ax1.plot(xs, ys, 'o-', color=colors_ang[idx], label=f'{ang}° twist', linewidth=2, markersize=4)

    ax1.axhline(1.0, color='gray', linestyle=':', alpha=0.6)
    ax1.axvline(0.0, color='black', linestyle='--', alpha=0.4, label='Joint center (x=0)')
    ax1.set_xlabel('Bar Longitudinal Axis X (joint at x=0)')
    ax1.set_ylabel('Per-Slice Thickness Ratio $r_{skinned} / r_{rest}$')
    ax1.set_title('LBS: Radial Shrinkage Along Bar (Candy-Wrapper Artifact)')
    ax1.legend(loc='lower left')
    ax1.grid(True, linestyle='--', alpha=0.5)
    ax1.set_ylim(0.20, 1.05)

    # DQS panel
    for idx, ang in enumerate(target_angles):
        if ('DQS', ang) in data:
            pts = sorted(data[('DQS', ang)], key=lambda p: p[0])
            xs = [p[0] for p in pts]
            ys = [p[1] for p in pts]
            ax2.plot(xs, ys, 's-', color=colors_ang[idx], label=f'{ang}° twist', linewidth=2, markersize=4)

    ax2.axhline(1.0, color='gray', linestyle=':', alpha=0.6)
    ax2.axvline(0.0, color='black', linestyle='--', alpha=0.4, label='Joint center (x=0)')
    ax2.set_xlabel('Bar Longitudinal Axis X (joint at x=0)')
    ax2.set_title('DQS: Radial Distance Ratio Along Bar (No Shrinkage)')
    ax2.legend(loc='lower left')
    ax2.grid(True, linestyle='--', alpha=0.5)

    plt.tight_layout()
    out1 = os.path.join(PLOTS_DIR, "plot2_twist_slices.png")
    out2 = os.path.join(RESULTS_DIR, "plot2_twist_slices.png")
    fig.savefig(out1)
    fig.savefig(out2)
    plt.close(fig)
    print(f"[Plot] Saved {out1}")

# ==============================================================================
# Plot 3: Bend Volume Ratio and Bulging Comparison
# ==============================================================================
def plot_bend_metrics():
    csv_path = os.path.join(RESULTS_DIR, "e1_quality.csv")
    if not os.path.exists(csv_path):
        csv_path = "e1_quality.csv"
    if not os.path.exists(csv_path):
        print(f"[Warning] {csv_path} not found. Skipping plot 3.")
        return

    angles_lbs, vols_lbs, min_thick_lbs, max_thick_lbs = [], [], [], []
    angles_dqs, vols_dqs, min_thick_dqs, max_thick_dqs = [], [], [], []

    with open(csv_path, 'r') as f:
        reader = csv.DictReader(f)
        for row in reader:
            if row['motion'] == 'Bend':
                m = row['method']
                deg = float(row['angle_deg'])
                vr = float(row['volume_ratio'])
                min_t = float(row['min_thickness_ratio']) if 'min_thickness_ratio' in row else 1.0
                max_t = float(row['max_thickness_ratio']) if 'max_thickness_ratio' in row else 1.0
                if m == 'LBS':
                    angles_lbs.append(deg)
                    vols_lbs.append(vr)
                    min_thick_lbs.append(min_t)
                    max_thick_lbs.append(max_t)
                elif m == 'DQS':
                    angles_dqs.append(deg)
                    vols_dqs.append(vr)
                    min_thick_dqs.append(min_t)
                    max_thick_dqs.append(max_t)

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5.5), dpi=300)

    # Subplot 1: Volume ratio under bend
    ax1.plot(angles_lbs, vols_lbs, 'o-', color=COLORS['LBS'], label='LBS (Volume Collapse)')
    ax1.plot(angles_dqs, vols_dqs, 's-', color=COLORS['DQS'], label='DQS (Volume Preserved)')
    ax1.axhline(1.0, color='gray', linestyle=':', alpha=0.6)
    ax1.set_xlabel('Bend Angle (degrees)')
    ax1.set_ylabel('Volume Ratio $V(\\theta) / V_0$')
    ax1.set_title('Bend Motion: Volume Preservation')
    ax1.legend(loc='lower left')
    ax1.grid(True, linestyle='--', alpha=0.5)
    ax1.set_xticks(np.arange(0, 135, 15))
    ax1.set_ylim(0.92, 1.01)

    # Subplot 2: Joint Profile & Bulging (Rotation-Invariant Thickness)
    ax2.plot(angles_lbs, min_thick_lbs, 'o-', color=COLORS['LBS'], label='LBS Min Thickness (Pinching)')
    ax2.plot(angles_dqs, min_thick_dqs, 's--', color=COLORS['DQS'], label='DQS Min Thickness (Preserved)')
    ax2.plot(angles_dqs, max_thick_dqs, '^-', color='#9467bd', label='DQS Max Thickness (Bulge)')
    ax2.axhline(1.0, color='gray', linestyle=':', alpha=0.6, label='Rest Thickness (1.0)')
    ax2.set_xlabel('Bend Angle (degrees)')
    ax2.set_ylabel('Rotation-Invariant Thickness Ratio')
    ax2.set_title('Bend Motion: Joint Profile & Bulging (Rotation-Invariant)')
    ax2.legend(loc='lower left')
    ax2.grid(True, linestyle='--', alpha=0.5)
    ax2.set_xticks(np.arange(0, 135, 15))
    ax2.set_ylim(0.75, 1.05)

    plt.tight_layout()
    out1 = os.path.join(PLOTS_DIR, "plot3_bend_volume_bulge.png")
    out2 = os.path.join(RESULTS_DIR, "plot3_bend_volume_bulge.png")
    fig.savefig(out1)
    fig.savefig(out2)
    plt.close(fig)
    print(f"[Plot] Saved {out1}")

# ==============================================================================
# Plot 4: GPU Time vs Vertex Count (Log Scale)
# ==============================================================================
def plot_gpu_vs_vertices():
    csv_path = os.path.join(RESULTS_DIR, "e2_gpu.csv")
    if not os.path.exists(csv_path):
        csv_path = "e2_gpu.csv"
    if not os.path.exists(csv_path):
        print(f"[Warning] {csv_path} not found. Skipping plot 4.")
        return

    # Filter: bones=60, influences=4, dataset=D3
    # Group by variant -> {variant: {verts: [gpu_ms, ...]}}
    data = {}
    with open(csv_path, 'r') as f:
        reader = csv.DictReader(f)
        for row in reader:
            if row['dataset'] == 'D3' and int(row['bones']) == 60 and int(row['influences']) == 4:
                v = row['variant']
                vc = int(row['vertices'])
                ms = float(row['gpu_ms_median'])
                if v not in data:
                    data[v] = {}
                if vc not in data[v]:
                    data[v][vc] = []
                data[v][vc].append(ms)

    fig, ax = plt.subplots(figsize=(8.5, 6), dpi=300)

    markers = {'LBS-naive': 'o', 'LBS-pre': '^', 'DQS-naive': 's', 'DQS-pre': 'D'}
    variants_order = ['LBS-naive', 'LBS-pre', 'DQS-naive', 'DQS-pre']

    for v in variants_order:
        if v in data:
            v_data = data[v]
            vcs = sorted(v_data.keys())
            medians = [np.median(v_data[vc]) for vc in vcs]
            p5 = [np.percentile(v_data[vc], 5) for vc in vcs]
            p95 = [np.percentile(v_data[vc], 95) for vc in vcs]
            err_lo = [m - p for m, p in zip(medians, p5)]
            err_hi = [p - m for m, p in zip(medians, p95)]

            ax.errorbar(vcs, medians, yerr=[err_lo, err_hi], fmt=f'{markers.get(v,"o")}-',
                        color=COLORS.get(v, '#333'), label=v, capsize=4, elinewidth=1.5)

    ax.set_xscale('log')
    ax.set_yscale('log')
    ax.set_xlabel('Vertex Count (log scale)')
    ax.set_ylabel('GPU Time per Frame (ms, log scale)')
    ax.set_title('GPU Skinning Throughput vs Vertex Count\n(60 Bones, 4 Influences, GL_RASTERIZER_DISCARD)')
    ax.legend(loc='upper left', frameon=True)
    ax.grid(True, which='both', linestyle='--', alpha=0.5)

    plt.tight_layout()
    out1 = os.path.join(PLOTS_DIR, "plot4_gpu_vs_vertices.png")
    out2 = os.path.join(RESULTS_DIR, "plot4_gpu_vs_vertices.png")
    fig.savefig(out1)
    fig.savefig(out2)
    plt.close(fig)
    print(f"[Plot] Saved {out1}")

# ==============================================================================
# Plot 5: GPU Time vs Bone Count and Influence Count
# ==============================================================================
def plot_gpu_bones_influences():
    csv_path = os.path.join(RESULTS_DIR, "e2_gpu.csv")
    if not os.path.exists(csv_path):
        csv_path = "e2_gpu.csv"
    if not os.path.exists(csv_path):
        print(f"[Warning] {csv_path} not found. Skipping plot 5.")
        return

    # Subplot A: verts=100000, influences=4, x=bones (20, 60, 120)
    # Subplot B: verts=100000, bones=60, x=influences (1, 2, 4)
    data_bones = {}
    data_influences = {}

    with open(csv_path, 'r') as f:
        reader = csv.DictReader(f)
        for row in reader:
            if row['dataset'] == 'D3':
                v = row['variant']
                vc = int(row['vertices'])
                bc = int(row['bones'])
                ic = int(row['influences'])
                ms = float(row['gpu_ms_median'])

                # Filter for bone scaling: 100k verts, 4 influences
                if 90000 <= vc <= 110000 and ic == 4:
                    if v not in data_bones: data_bones[v] = {}
                    if bc not in data_bones[v]: data_bones[v][bc] = []
                    data_bones[v][bc].append(ms)

                # Filter for influence scaling: 100k verts, 60 bones
                if 90000 <= vc <= 110000 and bc == 60:
                    if v not in data_influences: data_influences[v] = {}
                    if ic not in data_influences[v]: data_influences[v][ic] = []
                    data_influences[v][ic].append(ms)

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5.5), dpi=300)
    markers = {'LBS-naive': 'o', 'LBS-pre': '^', 'DQS-naive': 's', 'DQS-pre': 'D'}
    variants_order = ['LBS-naive', 'LBS-pre', 'DQS-naive', 'DQS-pre']

    # Subplot 1: vs Bones
    for v in variants_order:
        if v in data_bones:
            bcs = sorted(data_bones[v].keys())
            meds = [np.median(data_bones[v][b]) for b in bcs]
            ax1.plot(bcs, meds, f'{markers.get(v,"o")}-', color=COLORS.get(v, '#333'), label=v)

    ax1.set_xlabel('Bone Count (20, 60, 120)')
    ax1.set_ylabel('GPU Time (ms)')
    ax1.set_title('GPU Time vs Bone Count (100k Verts, 4 Influences)')
    ax1.legend(loc='best')
    ax1.grid(True, linestyle='--', alpha=0.5)

    # Subplot 2: vs Influences
    for v in variants_order:
        if v in data_influences:
            ics = sorted(data_influences[v].keys())
            meds = [np.median(data_influences[v][i]) for i in ics]
            ax2.plot(ics, meds, f'{markers.get(v,"o")}-', color=COLORS.get(v, '#333'), label=v)

    ax2.set_xlabel('Influences per Vertex (1, 2, 4)')
    ax2.set_ylabel('GPU Time (ms)')
    ax2.set_title('GPU Time vs Influences per Vertex (100k Verts, 60 Bones)')
    ax2.set_xticks([1, 2, 4])
    ax2.legend(loc='best')
    ax2.grid(True, linestyle='--', alpha=0.5)

    plt.tight_layout()
    out1 = os.path.join(PLOTS_DIR, "plot5_gpu_bones_influences.png")
    out2 = os.path.join(RESULTS_DIR, "plot5_gpu_bones_influences.png")
    fig.savefig(out1)
    fig.savefig(out2)
    plt.close(fig)
    print(f"[Plot] Saved {out1}")

# ==============================================================================
# Plot 6: Upload Paths (GPU Time + Upload Time vs Bone Count)
# ==============================================================================
def plot_upload_paths():
    csv_path = os.path.join(RESULTS_DIR, "e3_path.csv")
    if not os.path.exists(csv_path):
        csv_path = "e3_path.csv"
    if not os.path.exists(csv_path):
        print(f"[Warning] {csv_path} not found. Skipping plot 6.")
        return

    # Data structure: {(variant, path): {bones: [(gpu_ms, upload_ms, status), ...]}}
    data = {}
    with open(csv_path, 'r') as f:
        reader = csv.DictReader(f)
        for row in reader:
            v = row['variant']
            p = row['path']
            bc = int(row['bones'])
            gpu = float(row['gpu_ms_median'])
            up = float(row['upload_ms'])
            st = row['status']
            key = (v, p)
            if key not in data:
                data[key] = {}
            if bc not in data[key]:
                data[key][bc] = []
            data[key][bc].append((gpu, up, st))

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(15, 6), dpi=300, sharey=True)

    paths_order = ['uniform', 'ubo', 'tbo']
    path_colors = {'uniform': COLORS['Uniform'], 'ubo': COLORS['UBO'], 'tbo': COLORS['TBO']}
    path_markers = {'uniform': 'o', 'ubo': 's', 'tbo': '^'}

    # Panel 1: LBS-pre
    for p in paths_order:
        key = ('LBS-pre', p)
        if key in data:
            bcs = sorted(data[key].keys())
            x_valid, y_gpu, y_total = [], [], []
            x_exceed = []
            for b in bcs:
                runs = data[key][b]
                status = runs[0][2]
                if status == 'exceeds_limit':
                    x_exceed.append(b)
                else:
                    g_med = np.median([r[0] for r in runs])
                    u_med = np.median([r[1] for r in runs])
                    x_valid.append(b)
                    y_gpu.append(g_med)
                    y_total.append(g_med + u_med)

            if x_valid:
                ax1.plot(x_valid, y_total, f'{path_markers[p]}-', color=path_colors[p],
                         label=f'{p.upper()} (GPU + upload)', linewidth=2.2)
                ax1.plot(x_valid, y_gpu, f'{path_markers[p]}:', color=path_colors[p],
                         alpha=0.6, label=f'{p.upper()} (GPU only)')
            for ex_b in x_exceed:
                ax1.scatter([ex_b], [0.08], marker='x', color='red', s=100, zorder=5)
                ax1.annotate('Exceeds\nLimit', xy=(ex_b, 0.08), xytext=(ex_b, 0.12),
                             ha='center', fontsize=8, color='red', weight='bold')

    ax1.set_xlabel('Bone Count')
    ax1.set_ylabel('Time per Frame (ms)')
    ax1.set_title('LBS-pre: Upload Path Performance (100k Verts, 4 Influences)')
    ax1.set_xticks([60, 120, 256, 512])
    ax1.legend(loc='upper left', fontsize=9)
    ax1.grid(True, linestyle='--', alpha=0.5)
    ax1.set_ylim(bottom=0.0)

    # Panel 2: DQS-pre
    for p in paths_order:
        key = ('DQS-pre', p)
        if key in data:
            bcs = sorted(data[key].keys())
            x_valid, y_gpu, y_total = [], [], []
            x_exceed = []
            for b in bcs:
                runs = data[key][b]
                status = runs[0][2]
                if status == 'exceeds_limit':
                    x_exceed.append(b)
                else:
                    g_med = np.median([r[0] for r in runs])
                    u_med = np.median([r[1] for r in runs])
                    x_valid.append(b)
                    y_gpu.append(g_med)
                    y_total.append(g_med + u_med)

            if x_valid:
                ax2.plot(x_valid, y_total, f'{path_markers[p]}-', color=path_colors[p],
                         label=f'{p.upper()} (GPU + upload)', linewidth=2.2)
                ax2.plot(x_valid, y_gpu, f'{path_markers[p]}:', color=path_colors[p],
                         alpha=0.6, label=f'{p.upper()} (GPU only)')
            for ex_b in x_exceed:
                ax2.scatter([ex_b], [0.08], marker='x', color='red', s=100, zorder=5)
                ax2.annotate('Exceeds\nLimit', xy=(ex_b, 0.08), xytext=(ex_b, 0.11),
                             ha='center', fontsize=8, color='red', weight='bold')

    ax2.set_xlabel('Bone Count')
    ax2.set_title('DQS-pre: Upload Path Performance (100k Verts, 4 Influences)')
    ax2.set_xticks([60, 120, 256, 512])
    ax2.legend(loc='upper left', fontsize=9)
    ax2.grid(True, linestyle='--', alpha=0.5)

    plt.tight_layout()
    out1 = os.path.join(PLOTS_DIR, "plot6_upload_paths.png")
    out2 = os.path.join(RESULTS_DIR, "plot6_upload_paths.png")
    fig.savefig(out1)
    fig.savefig(out2)
    plt.close(fig)
    print(f"[Plot] Saved {out1}")

# ==============================================================================
# Plot 7: Woman.gltf Benchmark Comparison
# ==============================================================================
def plot_woman_comparison():
    csv_path = os.path.join(RESULTS_DIR, "e2_gpu.csv")
    if not os.path.exists(csv_path):
        csv_path = "e2_gpu.csv"
    if not os.path.exists(csv_path):
        print(f"[Warning] {csv_path} not found. Skipping plot 7.")
        return

    data = {}
    with open(csv_path, 'r') as f:
        reader = csv.DictReader(f)
        for row in reader:
            if row['dataset'] == 'Woman':
                v = row['variant']
                gpu = float(row['gpu_ms_median'])
                cpu = float(row['cpu_palette_ms'])
                up = float(row['upload_ms'])
                if v not in data:
                    data[v] = {'gpu': [], 'cpu': [], 'up': []}
                data[v]['gpu'].append(gpu)
                data[v]['cpu'].append(cpu)
                data[v]['up'].append(up)

    if not data:
        print("[Info] No Woman.gltf data in e2_gpu.csv. Skipping plot 7.")
        return

    variants = [v for v in ['LBS-naive', 'LBS-pre', 'DQS-naive', 'DQS-pre'] if v in data]
    gpu_meds = [np.median(data[v]['gpu']) for v in variants]
    cpu_meds = [np.median(data[v]['cpu']) for v in variants]
    up_meds  = [np.median(data[v]['up'])  for v in variants]

    fig, ax = plt.subplots(figsize=(8.5, 5.5), dpi=300)
    x = np.arange(len(variants))
    width = 0.25

    rects1 = ax.bar(x - width, gpu_meds, width, label='GPU Time (GL Timer)', color='#2b5c8f')
    rects2 = ax.bar(x,         cpu_meds, width, label='CPU Palette (QPC)', color='#e27c38')
    rects3 = ax.bar(x + width, up_meds,  width, label='Upload (QPC)', color='#419945')

    ax.set_ylabel('Median Time per Frame (ms)')
    ax.set_title('Production Model Benchmark: Woman.gltf (5,718 Vertices, 41 Bones)')
    ax.set_xticks(x)
    ax.set_xticklabels(variants, weight='bold')
    ax.legend(loc='upper right', frameon=True)
    ax.grid(True, axis='y', linestyle='--', alpha=0.5)

    for rects in [rects1, rects2, rects3]:
        for rect in rects:
            height = rect.get_height()
            if height > 0.0001:
                ax.annotate(f'{height:.4f}',
                            xy=(rect.get_x() + rect.get_width() / 2, height),
                            xytext=(0, 3), textcoords="offset points",
                            ha='center', va='bottom', fontsize=8)

    plt.tight_layout()
    out1 = os.path.join(PLOTS_DIR, "plot7_woman_comparison.png")
    out2 = os.path.join(RESULTS_DIR, "plot7_woman_comparison.png")
    fig.savefig(out1)
    fig.savefig(out2)
    plt.close(fig)
    print(f"[Plot] Saved {out1}")

if __name__ == '__main__':
    print("Generating all benchmark plots from CSV files...")
    plot_twist_volume()
    plot_twist_slices()
    plot_bend_metrics()
    plot_gpu_vs_vertices()
    plot_gpu_bones_influences()
    plot_upload_paths()
    plot_woman_comparison()
    print("Done generating plots.")
