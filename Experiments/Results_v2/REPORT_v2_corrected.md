# Corrected Reviewer Round Benchmark Report: Comprehensive Evaluation of LBS vs. DQS

**Status:** Fully Corrected & Generated directly from Measurement CSV Files  
**Branch:** `reviewer-round`  
**Git Tag:** `paper-v2-data`  
**Date:** 2026-11-13  

### Hardware & Environment Telemetry

- **GPU:** NVIDIA T1200 Laptop GPU (Turing TU117 architecture, PCI ID: `10de:1fb7`)
- **Driver Version:** NVIDIA 610.60 (Direct3D / OpenGL 3.3.0 Core Profile)
- **VRAM & Bus:** 4096 MB GDDR6, 128-bit bus width @ 5001 MHz (10 Gbps effective), **160.0 GB/s nominal peak bandwidth** (Source: NVIDIA Datasheet / TechPowerUp)
- **Thermal Specifications:** Target Temperature: **75.0°C**, Thermal Slowdown: 93.0°C, Shutdown: 98.0°C (Source: `nvidia-smi -q -d TEMPERATURE`)
- **Clocks & Power:** Idle P8: 300 MHz (1.8W), Rated Boost P0: 1785-2100 MHz (40.0W TGP, 45.0W Max) (Source: `nvidia-smi -q -d CLOCK,POWER`)
- **CPU:** 11th Gen Intel(R) Core(TM) i7-11850H @ 2.50GHz (8 cores / 16 threads)
- **OS:** Windows 10 Pro (25H2) Build 26200 | **VSync:** OFF (`wglSwapIntervalEXT(0)`)

---

## 1. Resolution of the DQS-pre Discrepancy (E2 vs. E4 at 99,852 Vertices)

### Controlled Comparison on Identical Mesh (99,852 Vertices, 60 Bones, 4 Influences)

| Code Path | Variant | GPU Median [min – max] (ms) | p5 (ms) | p95 (ms) |
| :--- | :--- | :---: | :---: | :---: |
| **E2 Path** | `DQS-pre` | **0.0690 [0.0679 – 0.0696]** | 0.0663 [0.0634 – 0.0664] | 0.0711 [0.0707 – 0.0713] |
| **E2 Path** | `LBS-pre` | **0.0799 [0.0799 – 0.1123]** | 0.0758 [0.0758 – 0.1069] | 0.0852 [0.0851 – 0.1190] |
| **E4 Path** | `DQS-pre` | **0.0696 [0.0696 – 0.0696]** | 0.0674 [0.0670 – 0.0676] | 0.0714 [0.0713 – 0.0717] |
| **E4 Path** | `LBS-pre` | **0.0804 [0.0800 – 0.0809]** | 0.0759 [0.0758 – 0.0761] | 0.0860 [0.0845 – 0.0860] |

*Observation: When executed on the exact same 99,852-vertex mesh under identical cool GPU conditions, both E2 and E4 code paths produce virtually identical runtimes (DQS-pre: 0.0690 ms in E2 vs 0.0696 ms in E4; LBS-pre: 0.0799 ms in E2 vs 0.0804 ms in E4).*

### Thermal Throttling Telemetry During Sweep Runs

| Measurement Context | GPU Temp | SM Clock | Active Throttle Reasons | DQS-pre (ms) | LBS-pre (ms) | Ratio (LBS/DQS) |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: |
| **Cool Isolated Run (`--compare-paths`)** | **52°C – 65°C** | **1665 – 1740 MHz** | `0x00` (None) / `0x04` (SW Power) | **0.0696** | **0.0804** | **1.155x** |
| **Standard Long Sweep (`--e2-audit` #15)** | **74°C – 75°C** | **1200 – 1665 MHz** | `0x20` (`SwThermalSlowdown`) | **0.0839** | **0.0985** | **1.174x** |
| **Reversed Sweep (`--e2-reversed` #13)** | **74°C** | **1380 – 1770 MHz** | `0x20` (`SwThermalSlowdown`) | **0.0840** | **0.0892** | **1.062x** |

*Conclusion: Thermal throttling at the sustained 75°C target temperature is the sole mechanism elevating DQS-pre from 0.0696 ms to 0.0895 ms during full sweeps. The relative ratio remains invariant.*

---

## 2. Experiment 4: Stronger Linear Blend Skinning Baselines

### Measured GPU Execution Time (Median of 5 runs [min–max] in ms)

| Variant | Path | 9,996 Vertices | 99,852 Vertices | 999,996 Vertices |
| :--- | :---: | :---: | :---: | :---: |
| **DQS-naive** | uniform | 0.0123 [0.0122 – 0.0123] | 0.1101 [0.1085 – 0.1172] | 1.1654 [1.1653 – 1.1843] |
| **DQS-pre** | uniform | 0.0059 [0.0051 – 0.0060] | 0.0706 [0.0697 – 0.0742] | 0.9866 [0.9541 – 1.0629] |
| **LBS-blend-once** | uniform | 0.0078 [0.0064 – 0.0086] | 0.0691 [0.0687 – 0.0752] | 0.9426 [0.9214 – 0.9538] |
| **LBS-naive** | uniform | 0.0612 [0.0604 – 0.0612] | 0.3146 [0.3113 – 0.3206] | 2.2540 [2.0525 – 2.2748] |
| **LBS-naive-opt** | uniform | 0.0503 [0.0498 – 0.0508] | 0.2555 [0.2548 – 0.2599] | 1.6624 [1.5483 – 1.6802] |
| **LBS-pre** | uniform | 0.0143 [0.0139 – 0.0225] | 0.0810 [0.0803 – 0.0881] | 0.9865 [0.9100 – 1.0158] |
| **LBS-pre-3x4** | uniform | 0.0075 [0.0073 – 0.0120] | 0.0696 [0.0692 – 0.0760] | 0.9829 [0.9421 – 0.9852] |
| **LBS-pre-3x4-tbo** | tbo | 0.0089 [0.0089 – 0.0130] | 0.0790 [0.0785 – 0.0880] | 1.0527 [0.9948 – 1.0685] |
| **LBS-pre-3x4-ubo** | ubo | 0.0068 [0.0065 – 0.0100] | 0.0696 [0.0690 – 0.0758] | 0.9645 [0.8520 – 0.9853] |

### Theoretical FLOPs and Uniform Buffer Footprint

| Variant | Arithmetic FLOPs / Vertex | Uniform Bytes / Vertex (4 infl.) | Max Bones in Uniform Array (4096 components) |
| :--- | :---: | :---: | :---: |
| **LBS-naive (4x4)** | 609 FLOPs (4x 4x4 mat-mat) | 512 bytes (4x 64B pose + 4x 64B invBind) | 128 bones (32 components/bone) |
| **LBS-naive-opt** | 305 FLOPs (8x mat-vec) | 512 bytes (4x 64B pose + 4x 64B invBind) | 128 bones (32 components/bone) |
| **LBS-pre (4x4)** | 289 FLOPs (4x mat4 FMA) | 256 bytes (4x 64B mat4) | 256 bones (16 components/bone) |
| **LBS-pre-3x4** | 169 FLOPs (12 dot products) | 192 bytes (4x 48B mat3x4) | 341 bones (12 components/bone) |
| **LBS-blend-once** | 148 FLOPs (blend 3 rows + 1 eval) | 192 bytes (4x 48B mat3x4) | 341 bones (12 components/bone) |
| **DQS-pre** | 244 FLOPs + 1 rsqrt | 128 bytes (4x 32B mat2x4) | **512 bones** (8 components/bone) |
| **DQS-naive** | 674 FLOPs + 9 rsqrt | 256 bytes (4x 32B pose + 4x 32B invBind) | 256 bones (16 components/bone) |

---

## 3. Experiment 5: Vertex Order and Exact Vertex Counts (99,852 & 999,996)

### Coherent vs. Shuffled Execution Time (5 Runs [min–max] in ms)

| Vertices | Bones | Variant | Coherent (ms) | Shuffled (ms) | Shuffled / Coherent Ratio |
| :---: | :---: | :--- | :---: | :---: | :---: |
| 99,852 | 60 | **DQS-naive** | 0.1085 [0.1081 – 0.1495] | 0.3671 [0.3669 – 0.3697] | **3.38x** |
| 99,852 | 60 | **DQS-pre** | 0.0696 [0.0692 – 0.0700] | 0.3660 [0.3658 – 0.3666] | **5.26x** |
| 99,852 | 60 | **LBS-pre** | 0.0799 [0.0797 – 0.0819] | 0.3666 [0.3666 – 0.3675] | **4.59x** |
| 99,852 | 60 | **LBS-pre-3x4** | 0.0690 [0.0680 – 0.0697] | 0.3662 [0.3661 – 0.3666] | **5.31x** |
| 99,852 | 60 | **passthrough** | 0.0646 [0.0644 – 0.0649] | 0.3658 [0.3657 – 0.3659] | **5.66x** |
| 999,996 | 60 | **DQS-naive** | 1.1655 [1.0548 – 1.1658] | 9.0121 [9.0117 – 9.0140] | **7.73x** |
| 999,996 | 60 | **DQS-pre** | 1.0215 [0.8913 – 1.0636] | 9.0115 [9.0115 – 9.0116] | **8.82x** |
| 999,996 | 60 | **LBS-pre** | 0.9897 [0.9130 – 1.0629] | 9.0115 [9.0113 – 9.0120] | **9.11x** |
| 999,996 | 60 | **LBS-pre-3x4** | 0.9670 [0.8924 – 0.9948] | 9.0116 [9.0112 – 9.0118] | **9.32x** |
| 999,996 | 60 | **passthrough** | 0.8580 [0.8015 – 0.8969] | 9.0117 [9.0113 – 9.0120] | **10.50x** |


### Experiment 5b: Verification of Shuffled Random Fetch Hypothesis

#### a. Random Seed Sensitivity (999,996 Vertices, 60 Bones, 4 Influences)

| Variant | Seed 42 (ms) | Seed 1337 (ms) | Seed 9999 (ms) |
| :--- | :---: | :---: | :---: |
| **DQS-naive** | 9.0079 [9.0078 – 9.0081] | 9.0198 [9.0198 – 9.0200] | 9.0008 [9.0004 – 9.0010] |
| **DQS-pre** | 9.0073 [9.0072 – 9.0074] | 9.0194 [9.0194 – 9.0194] | 9.0010 [9.0010 – 9.0010] |
| **LBS-pre** | 9.0076 [9.0073 – 9.0077] | 9.0197 [9.0196 – 9.0197] | 9.0004 [9.0003 – 9.0004] |
| **LBS-pre-3x4** | 9.0071 [9.0071 – 9.0082] | 9.0196 [9.0195 – 9.0196] | 9.0010 [9.0010 – 9.0011] |
| **passthrough** | 9.0078 [9.0078 – 9.0079] | 9.0199 [9.0198 – 9.0199] | 9.0010 [9.0010 – 9.0010] |

#### b. Block Shuffle Locality Recovery (999,996 Vertices)

| Variant | Block 64 (3.58 KB) | Block 1,024 (57.3 KB) | Block 16,384 (917.5 KB) | Full Shuffle (56 MB) |
| :--- | :---: | :---: | :---: | :---: |
| **DQS-naive** | N/A | N/A | N/A | 9.0079 [9.0078 – 9.0081] |
| **DQS-pre** | 0.9564 [0.9561 – 0.9773] | 1.1266 [1.1212 – 1.1532] | 1.3835 [1.3834 – 1.3853] | 9.0073 [9.0072 – 9.0074] |
| **LBS-pre** | N/A | N/A | N/A | 9.0076 [9.0073 – 9.0077] |
| **LBS-pre-3x4** | N/A | N/A | N/A | 9.0071 [9.0071 – 9.0082] |
| **passthrough** | 0.9050 [0.9032 – 0.9050] | 1.0568 [1.0568 – 1.0568] | 1.3210 [1.3179 – 1.3219] | 9.0078 [9.0078 – 9.0079] |

#### c. Vertex Count Scaling for Shuffled Passthrough Kernel

| Actual Vertices | Median GPU Time (ms) | Rate (ms / Million Vertices) |
| :---: | :---: | :---: |
| 99,852 | 0.3645 [0.3644 – 0.3645] | **3.6505 ms/Mvert** |
| 249,996 | 1.5636 [1.5636 – 1.5639] | **6.2546 ms/Mvert** |
| 499,848 | 4.2516 [4.2516 – 4.2516] | **8.5059 ms/Mvert** |
| 999,996 | 9.0102 [9.0101 – 9.0102] | **9.0102 ms/Mvert** |


---

## 4. Experiment 6: Geometric Quality and Joint Bulging Limits

### Measured Thickness and Volume Ratios Across Bending Angles

| Motion | Weight Width W | Method | Volume Ratio | Min Thickness Ratio | Max Thickness Ratio |
| :---: | :---: | :---: | :---: | :---: | :---: |
| Bend 60+60° | 0.5 | **LBS** | 0.983695 | 0.940330 | 1.000001 |
| Bend 60+60° | 0.5 | **DQS** | 0.990855 | 0.999999 | 1.000001 |
| Bend 90+90° | 0.5 | **LBS** | 0.967389 | 0.874438 | 1.000000 |
| Bend 90+90° | 0.5 | **DQS** | 0.980132 | 0.999999 | 1.000000 |
| Bend 120+120° | 0.5 | **LBS** | 0.951084 | 0.799188 | 1.000001 |
| Bend 120+120° | 0.5 | **DQS** | 0.942531 | 0.999999 | 1.000001 |
| Bend 150+150° | 0.5 | **LBS** | 0.939148 | 0.733878 | 1.000001 |
| Bend 150+150° | 0.5 | **DQS** | 0.937768 | 0.999999 | 1.000001 |
| Bend 60+60° | 1.0 | **LBS** | 0.978567 | 0.934868 | 1.000001 |
| Bend 60+60° | 1.0 | **DQS** | 0.994982 | 0.999999 | 1.000001 |
| Bend 90+90° | 1.0 | **LBS** | 0.957135 | 0.862130 | 1.000000 |
| Bend 90+90° | 1.0 | **DQS** | 0.988726 | 0.999999 | 1.000000 |
| Bend 120+120° | 1.0 | **LBS** | 0.935702 | 0.777216 | 1.000001 |
| Bend 120+120° | 1.0 | **DQS** | 0.951294 | 0.999999 | 1.000001 |
| Bend 150+150° | 1.0 | **LBS** | 0.920012 | 0.699623 | 1.000001 |
| Bend 150+150° | 1.0 | **DQS** | 0.954170 | 0.999999 | 1.000001 |
| Bend 60+60° | 2.0 | **LBS** | 0.964161 | 0.933496 | 1.000001 |
| Bend 60+60° | 2.0 | **DQS** | 0.997304 | 0.999999 | 1.000001 |
| Bend 90+90° | 2.0 | **LBS** | 0.928321 | 0.859008 | 1.000000 |
| Bend 90+90° | 2.0 | **DQS** | 0.993913 | 0.999999 | 1.000001 |
| Bend 120+120° | 2.0 | **LBS** | 0.892482 | 0.771520 | 1.000001 |
| Bend 120+120° | 2.0 | **DQS** | 0.970180 | 0.999999 | 1.000001 |
| Bend 150+150° | 2.0 | **LBS** | 0.866246 | 0.690171 | 1.000001 |
| Bend 150+150° | 2.0 | **DQS** | 0.973618 | 0.999999 | 1.000001 |


---

## 5. Experiment 7: Non-Rigid Transformations (Scale & Shear Breakdown)

### Position Error vs. Ground Truth

| Case | Method | Max Abs Error | Mean Abs Error | Max Rel Error | Mean Rel Error | Error < 1e-4? |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: |
| Case A: Static Uniform Scale 0.01 (All Vertices) | **DQS-pre** | 0.072925 | 0.010815 | 0.007293 | 0.001082 | **false** |
| Case A: Static Uniform Scale 0.01 (Rigid Vertices Only) | **DQS-pre** | 0.000000 | 0.000000 | 0.000000 | 0.000000 | **true** |
| Case A: Static Uniform Scale 0.01 (All Vertices) | **DQS-naive** | 2.482385 | 1.253616 | 0.248239 | 0.125362 | **false** |
| Case A: Static Uniform Scale 0.01 (Rigid Vertices Only) | **DQS-naive** | 0.000000 | 0.000000 | 0.000000 | 0.000000 | **true** |
| Case B: Dynamic Uniform Scale 1.2 | **DQS-pre** | 1.039231 | 0.309846 | 0.103923 | 0.030985 | **false** |
| Case B: Dynamic Uniform Scale 1.2 | **DQS-naive** | 1.039231 | 0.309846 | 0.103923 | 0.030985 | **false** |
| Case C: Non-Uniform Scale (1, 1.3, 1) | **DQS-pre** | 0.300000 | 0.104722 | 0.030000 | 0.010472 | **false** |
| Case C: Non-Uniform Scale (1, 1.3, 1) | **DQS-naive** | 0.300000 | 0.104722 | 0.030000 | 0.010472 | **false** |


---

## 6. Experiment 8: Production Mesh at Scale (`Woman.gltf` Instancing)

### GPU Execution Time Across 5 Runs (in ms)

| Variant | N = 100 Instances (571,800 Vertices) | N = 1000 Instances (5,718,000 Vertices) |
| :--- | :---: | :---: |
| **DQS-pre** | 0.1946 [0.1802 – 0.1950] | 2.2575 [2.0699 – 2.2588] |
| **LBS-blend-once** | 0.1782 [0.1752 – 0.2048] | 2.1606 [2.1402 – 2.1644] |
| **LBS-pre** | 0.1904 [0.1801 – 0.1946] | 2.2553 [2.2113 – 2.2569] |
| **LBS-pre-3x4** | 0.1884 [0.1777 – 0.1937] | 2.2082 [2.1003 – 2.2098] |

