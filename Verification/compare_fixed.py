import pandas as pd
import numpy as np

# Load committed vs fixed
df_old = pd.read_csv('Experiments/Results/e2_gpu.csv')
df_new = pd.read_csv('Experiments/Results_fixed/e2_gpu.csv')

# Filter for D3, 60 bones, 4 influences
d3_old = df_old[(df_old['dataset'] == 'D3') & (df_old['bones'] == 60) & (df_old['influences'] == 4)]
d3_new = df_new[(df_new['dataset'] == 'D3') & (df_new['bones'] == 60) & (df_new['influences'] == 4)]

verts = sorted(d3_old['vertices'].unique())
variants = ['LBS-naive', 'LBS-pre', 'DQS-naive', 'DQS-pre']

print('=== 60 BONES, 4 INFLUENCES COMPARISON (COMMITTED vs FIXED) ===')
for vc in verts:
    print(f'\n--- Vertices: {vc} ---')
    for v in variants:
        runs_old = d3_old[(d3_old['vertices'] == vc) & (d3_old['variant'] == v)]['gpu_ms_median'].values
        runs_new = d3_new[(d3_new['vertices'] == vc) & (d3_new['variant'] == v)]['gpu_ms_median'].values
        
        med_old = np.median(runs_old)
        min_old, max_old = np.min(runs_old), np.max(runs_old)
        
        med_new = np.median(runs_new)
        min_new, max_new = np.min(runs_new), np.max(runs_new)
        
        pct_diff = ((med_new - med_old) / med_old) * 100.0
        print(f'{v:10s} | Old: {med_old:.4f} [{min_old:.4f}-{max_old:.4f}] | New: {med_new:.4f} [{min_new:.4f}-{max_new:.4f}] | Diff: {pct_diff:+.2f}%')

print('\n=== ORDER CHECK: LBS-pre vs DQS-pre ===')
for vc in verts:
    lbs_old = np.median(d3_old[(d3_old['vertices'] == vc) & (d3_old['variant'] == 'LBS-pre')]['gpu_ms_median'].values)
    dqs_old = np.median(d3_old[(d3_old['vertices'] == vc) & (d3_old['variant'] == 'DQS-pre')]['gpu_ms_median'].values)
    
    lbs_new = np.median(d3_new[(d3_new['vertices'] == vc) & (d3_new['variant'] == 'LBS-pre')]['gpu_ms_median'].values)
    dqs_new = np.median(d3_new[(d3_new['vertices'] == vc) & (d3_new['variant'] == 'DQS-pre')]['gpu_ms_median'].values)
    
    ord_old = "DQS-pre < LBS-pre" if dqs_old < lbs_old else "LBS-pre < DQS-pre"
    ord_new = "DQS-pre < LBS-pre" if dqs_new < lbs_new else "LBS-pre < DQS-pre"
    print(f'Verts: {vc:7d} | Old: LBS-pre={lbs_old:.4f}, DQS-pre={dqs_old:.4f} ({ord_old}) | New: LBS-pre={lbs_new:.4f}, DQS-pre={dqs_new:.4f} ({ord_new})')

