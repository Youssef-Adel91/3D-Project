import os
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

def main():
    csv_path = 'e1b_quality.csv'
    if not os.path.exists(csv_path):
        csv_path = os.path.join('Results', 'e1b_quality.csv')
    if not os.path.exists(csv_path):
        csv_path = os.path.join('Experiments', 'e1b_quality.csv')
    if not os.path.exists(csv_path):
        csv_path = os.path.join('Experiments', 'Results', 'e1b_quality.csv')

    print(f"Reading CSV from {csv_path}...")
    df = pd.read_csv(csv_path)

    # Filter for Twist 180 deg and Bend 120 deg
    t180 = df[(df['motion'] == 'twist') & (df['angle_deg'] == 180.0)]
    b120 = df[(df['motion'] == 'bend') & (df['angle_deg'] == 120.0)]

    lbs_t180 = t180[t180['method'] == 'LBS'].sort_values('W')
    dqs_t180 = t180[t180['method'] == 'DQS'].sort_values('W')

    lbs_b120 = b120[b120['method'] == 'LBS'].sort_values('W')
    dqs_b120 = b120[b120['method'] == 'DQS'].sort_values('W')

    fig, axes = plt.subplots(2, 2, figsize=(12, 10), dpi=300)
    plt.subplots_adjust(hspace=0.3, wspace=0.25)

    w_ticks = [0.25, 0.5, 1.0, 2.0, 4.0, 8.0]
    w_labels = ['0.25', '0.5', '1', '2', '4', '8']

    # Colors
    c_lbs = '#D9534F'  # red
    c_dqs = '#2E6DA4'  # blue
    c_max = '#5CB85C'  # green

    # 1. Twist 180: Volume Ratio vs W
    ax = axes[0, 0]
    ax.plot(lbs_t180['W'], lbs_t180['volume_ratio'], 'o-', color=c_lbs, linewidth=2, markersize=7, label='LBS')
    ax.plot(dqs_t180['W'], dqs_t180['volume_ratio'], 's-', color=c_dqs, linewidth=2, markersize=7, label='DQS')
    # Analytical line: 1 - W/15
    w_fine = np.linspace(0.25, 8.0, 100)
    ax.plot(w_fine, 1.0 - w_fine / 15.0, '--', color='#888888', label='Analytical LBS (1 - W/15)')
    ax.set_xscale('log', base=2)
    ax.set_xticks(w_ticks)
    ax.set_xticklabels(w_labels)
    ax.set_ylim(0.4, 1.05)
    ax.set_xlabel('Weight Ramp Width W (units)', fontsize=11, fontweight='bold')
    ax.set_ylabel('Volume Ratio V(180°) / V₀', fontsize=11, fontweight='bold')
    ax.set_title('(a) Twist 180°: Volume Ratio vs. Weight Width W', fontsize=12, fontweight='bold')
    ax.grid(True, which='both', linestyle=':', alpha=0.6)
    ax.legend(loc='lower left', frameon=True)

    # 2. Twist 180: Minimum Thickness Ratio vs W
    ax = axes[0, 1]
    ax.plot(lbs_t180['W'], lbs_t180['min_thickness'], 'o-', color=c_lbs, linewidth=2, markersize=7, label='LBS Min Thickness')
    ax.plot(dqs_t180['W'], dqs_t180['min_thickness'], 's-', color=c_dqs, linewidth=2, markersize=7, label='DQS Min Thickness')
    ax.set_xscale('log', base=2)
    ax.set_xticks(w_ticks)
    ax.set_xticklabels(w_labels)
    ax.set_ylim(-0.05, 1.1)
    ax.set_xlabel('Weight Ramp Width W (units)', fontsize=11, fontweight='bold')
    ax.set_ylabel('Minimum Thickness Ratio', fontsize=11, fontweight='bold')
    ax.set_title('(b) Twist 180°: Minimum Thickness Ratio vs. W', fontsize=12, fontweight='bold')
    ax.grid(True, which='both', linestyle=':', alpha=0.6)
    ax.legend(loc='center right', frameon=True)

    # 3. Bend 120: Volume Ratio vs W
    ax = axes[1, 0]
    ax.plot(lbs_b120['W'], lbs_b120['volume_ratio'], 'o-', color=c_lbs, linewidth=2, markersize=7, label='LBS')
    ax.plot(dqs_b120['W'], dqs_b120['volume_ratio'], 's-', color=c_dqs, linewidth=2, markersize=7, label='DQS')
    ax.set_xscale('log', base=2)
    ax.set_xticks(w_ticks)
    ax.set_xticklabels(w_labels)
    ax.set_ylim(0.75, 1.02)
    ax.set_xlabel('Weight Ramp Width W (units)', fontsize=11, fontweight='bold')
    ax.set_ylabel('Volume Ratio V(120°) / V₀', fontsize=11, fontweight='bold')
    ax.set_title('(c) Bend 120°: Volume Ratio vs. Weight Width W', fontsize=12, fontweight='bold')
    ax.grid(True, which='both', linestyle=':', alpha=0.6)
    ax.legend(loc='lower left', frameon=True)

    # 4. Bend 120: Thickness Ratios vs W (including DQS max thickness)
    ax = axes[1, 1]
    ax.plot(lbs_b120['W'], lbs_b120['min_thickness'], 'o-', color=c_lbs, linewidth=2, markersize=7, label='LBS Min Thickness')
    ax.plot(dqs_b120['W'], dqs_b120['min_thickness'], 's-', color=c_dqs, linewidth=2, markersize=7, label='DQS Min Thickness')
    ax.plot(dqs_b120['W'], dqs_b120['max_thickness'], '^--', color=c_max, linewidth=2, markersize=7, label='DQS Max Thickness (Bulging)')
    ax.axhline(1.01, color='orange', linestyle=':', linewidth=1.5, label='Bulging Threshold (1.01)')
    ax.set_xscale('log', base=2)
    ax.set_xticks(w_ticks)
    ax.set_xticklabels(w_labels)
    ax.set_ylim(0.70, 1.05)
    ax.set_xlabel('Weight Ramp Width W (units)', fontsize=11, fontweight='bold')
    ax.set_ylabel('Thickness Ratio', fontsize=11, fontweight='bold')
    ax.set_title('(d) Bend 120°: Thickness Ratios vs. Weight Width W', fontsize=12, fontweight='bold')
    ax.grid(True, which='both', linestyle=':', alpha=0.6)
    ax.legend(loc='center right', frameon=True)

    plt.suptitle('Experiment 1b: Sensitivity of Deformation Quality to Weight Smoothness W', fontsize=14, fontweight='bold', y=0.98)

    out_file1 = 'plot_e1b.png'
    plt.savefig(out_file1, bbox_inches='tight')
    print(f"Plot saved to {out_file1}")
    if os.path.exists('..') and os.path.isdir('..'):
        try:
            plt.savefig(os.path.join('..', 'plot_e1b.png'), bbox_inches='tight')
            print(f"Plot saved to ../plot_e1b.png")
        except Exception:
            pass

if __name__ == '__main__':
    main()
