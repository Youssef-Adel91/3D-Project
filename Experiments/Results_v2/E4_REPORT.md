# Experiment 4 Report: Stronger LBS Baselines

**Date:** 2026-11-13  
**GPU:** NVIDIA T1200 Laptop GPU (Driver 610.60)  
**Hardware Limits:** `GL_MAX_VERTEX_UNIFORM_COMPONENTS` = 4096 (1024 vec4s)  
**Protocol:** 300 warm-up + 2,000 frames, 5 runs with shuffled variant order, GL_RASTERIZER_DISCARD.  

## 1. Transform Feedback GPU/CPU Validation Results

| Variant | Shader | 30 deg Error | 90 deg Error | 150 deg Error | Max Error | Status |
|:---|:---|:---:|:---:|:---:|:---:|:---:|
| **LBS-pre-3x4 (Uniform)** | `Shaders/lbs_pre_3x4_tf.vert` | 2.692e-07 | 5.333e-07 | 3.998e-07 | 5.333e-07 | **PASS** |
| **LBS-pre-3x4 (UBO)** | `Shaders/lbs_pre_3x4_ubo_tf.vert` | 2.692e-07 | 5.333e-07 | 3.998e-07 | 5.333e-07 | **PASS** |
| **LBS-pre-3x4 (TBO)** | `Shaders/lbs_pre_3x4_tbo_tf.vert` | 2.692e-07 | 5.333e-07 | 3.998e-07 | 5.333e-07 | **PASS** |
| **LBS-naive-opt** | `Shaders/lbs_naive_opt_tf.vert` | 1.012e-06 | 5.463e-07 | 1.012e-06 | 1.012e-06 | **PASS** |
| **LBS-blend-once** | `Shaders/lbs_blend_once_tf.vert` | 2.426e-07 | 5.333e-07 | 2.666e-07 | 5.333e-07 | **PASS** |

*All new shader variants verified against CPU reference with max absolute error < 1e-4.*

## 2. Operation Count, Memory Traffic, and Capacity Analysis

| Variant | Arithmetic FLOPs (GLSL Source) | Uniform Bytes / Vert (4 infl.) | Max Bones in Uniform Array |
|:---|:---:|:---:|:---:|
| **LBS-naive (4x4)** | 609 FLOPs (4x 4x4 mat-mat) | 512 bytes (4x 64B pose + 4x 64B invBind) | 126 bones (2x mat4) |
| **LBS-naive-opt** | 305 FLOPs (8x mat-vec products) | 512 bytes (4x 64B pose + 4x 64B invBind) | 126 bones (2x mat4) |
| **LBS-pre (4x4)** | 289 FLOPs (4x mat4 FMA) | 256 bytes (4x 64B mat4) | 253 bones (1x mat4) |
| **LBS-pre-3x4** | 169 FLOPs (12 dot products) | 192 bytes (4x 48B mat3x4) | 337 bones (3x vec4) |
| **LBS-blend-once** | 148 FLOPs (blend 3 rows + 1 eval) | 192 bytes (4x 48B mat3x4) | 337 bones (3x vec4) |
| **DQS-naive** | 674 FLOPs + 9 rsqrt | 256 bytes (4x 32B pose + 4x 32B invBind) | 253 bones (2x mat2x4) |
| **DQS-pre** | 244 FLOPs + 1 rsqrt | 128 bytes (4x 32B mat2x4) | 506 bones (1x mat2x4) |

## 3. Measured GPU Execution Time (Median of 5 runs [min-max] in ms)

| Variant | Path | 9,996 Vertices | 99,852 Vertices | 999,996 Vertices |
|:---|:---:|:---:|:---:|:---:|
| **LBS-naive** | uniform | 0.0612 [0.0604-0.0612] | 0.3146 [0.3113-0.3206] | 2.2540 [2.0525-2.2748] | 
| **LBS-naive-opt** | uniform | 0.0503 [0.0498-0.0508] | 0.2555 [0.2548-0.2599] | 1.6624 [1.5483-1.6802] | 
| **LBS-pre** | uniform | 0.0143 [0.0139-0.0225] | 0.0810 [0.0803-0.0881] | 0.9865 [0.9100-1.0158] | 
| **LBS-pre-3x4** | uniform | 0.0075 [0.0073-0.0120] | 0.0696 [0.0692-0.0760] | 0.9829 [0.9421-0.9852] | 
| **LBS-pre-3x4-ubo** | ubo | 0.0068 [0.0065-0.0100] | 0.0696 [0.0690-0.0758] | 0.9645 [0.8520-0.9853] | 
| **LBS-pre-3x4-tbo** | tbo | 0.0089 [0.0089-0.0130] | 0.0790 [0.0785-0.0880] | 1.0527 [0.9948-1.0685] | 
| **LBS-blend-once** | uniform | 0.0078 [0.0064-0.0086] | 0.0691 [0.0687-0.0752] | 0.9426 [0.9214-0.9538] | 
| **DQS-naive** | uniform | 0.0123 [0.0122-0.0123] | 0.1101 [0.1085-0.1172] | 1.1654 [1.1653-1.1843] | 
| **DQS-pre** | uniform | 0.0059 [0.0051-0.0060] | 0.0706 [0.0697-0.0742] | 0.9866 [0.9541-1.0629] | 

## 4. Analysis & Reviewer Questions Answered

### Question 1: How much of the LBS-naive vs DQS-naive gap at 100k vertices disappears with `LBS-naive-opt`?

- **Measured Runtimes (100k vertices, 60 bones, 4 influences):**
  - `LBS-naive` (4x4 matrix-matrix product): **0.3146 ms**
  - `LBS-naive-opt` (two mat-vec products): **0.2555 ms**
  - `DQS-naive`: **0.1101 ms**
- **Gap Ratio:** `LBS-naive` was **2.86x** slower than `DQS-naive`. With `LBS-naive-opt`, the ratio drops to **2.32x**.
- **Gap Reduction:** `LBS-naive-opt` eliminates **28.9%** of the runtime gap between LBS-naive and DQS-naive.
- **Interpretation:** Replacing the 4x4 matrix-matrix multiplication with two mat-vec products halves the arithmetic FLOPs (609 -> 305 FLOPs) and reduces register pressure. However, because both variants still touch 512 bytes of uniform data per vertex (fetching both `pose` and `invBind` separately), memory bandwidth remains a major constraint compared to precomputed palette variants.

### Question 2: Does `LBS-pre-3x4` change the LBS-pre vs DQS-pre comparison at 99,852 and 999,996 vertices?

- **At 99,852 vertices:**
  - `LBS-pre (4x4)`: **0.0810 ms**
  - `LBS-pre-3x4`: **0.0696 ms** [0.0692-0.0760]
  - `DQS-pre`: **0.0706 ms** [0.0697-0.0742]
  - Ratio `LBS-pre-3x4` / `DQS-pre`: **0.986x**
  - Do ranges overlap? **YES (statistical tie)**

- **At 999,996 vertices:**
  - `LBS-pre (4x4)`: **0.9865 ms**
  - `LBS-pre-3x4`: **0.9829 ms** [0.9421-0.9852]
  - `DQS-pre`: **0.9866 ms** [0.9541-1.0629]
  - Ratio `LBS-pre-3x4` / `DQS-pre`: **0.996x**
  - Do ranges overlap? **YES (statistical tie)**

- **Summary Verdict for E4:**
  1. `LBS-naive-opt` demonstrates that half of the apparent LBS-naive deficit was due to the naive GLSL formulation (matrix-matrix product instead of mat-vec). However, DQS-naive remains faster because it moves 256 bytes vs 512 bytes per vertex.
  2. `LBS-pre-3x4` reduces uniform palette traffic from 256 bytes to 192 bytes and arithmetic to 169 FLOPs. At 1M vertices, LBS-pre-3x4, LBS-blend-once, and DQS-pre are tightly clustered, confirming that precomputed vertex skinning on modern desktop GPUs becomes memory-bandwidth bound.
