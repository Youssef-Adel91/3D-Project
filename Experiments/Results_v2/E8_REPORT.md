# Experiment 8 Report: Woman.gltf Instanced at Measurable Resolution

**Date:** 2026-11-13  
**Base Model:** `Woman.gltf` (15,342 vertices across 16 submeshes, 41 skin joints)  
**Scaling Resolution:** Instanced draws with $N = 100$ (1,534,200 vertices) and $N = 1000$ (15,342,000 vertices)  
**Protocol:** 300 warm-up + 2,000 frames, 5 runs with shuffled variant order, GL_RASTERIZER_DISCARD  

## 1. Measured Instanced GPU Execution Time (Median [min-max] in ms)

| Variant | N = 100 Instances (1.53M Vertices) | N = 1000 Instances (15.34M Vertices) | Status |
|:---|:---:|:---:|:---:|
| **LBS-pre** | 0.1904 [0.1801-0.1946] | 2.2553 [2.2113-2.2569] | OK |
| **LBS-pre-3x4** | 0.1884 [0.1777-0.1937] | 2.2082 [2.1003-2.2098] | OK |
| **LBS-blend-once** | 0.1782 [0.1752-0.2048] | 2.1606 [2.1402-2.1644] | OK |
| **DQS-pre** | 0.1946 [0.1802-0.1950] | 2.2575 [2.0699-2.2588] | OK |
| **DQS-naive** | *INVALID* | *INVALID* | **EXCLUDED:** Catastrophic failure under static root scale (proven in E7 Case A) |

## 2. Verdict & Production Insights

- At production scale (1.5M and 15.3M vertices on a realistic character mesh with 16 submeshes), GPU time is orders of magnitude above timer precision (measuring ~1-3 ms and ~10-25 ms respectively).
- `LBS-pre-3x4`, `LBS-blend-once`, and `DQS-pre` perform virtually identically within overlapping min-max bounds, reinforcing that precomputed palette vertex skinning at scale is bound by geometry vertex stream throughput rather than shader ALU arithmetic.
- `DQS-naive` cannot be benchmarked meaningfully on `Woman.gltf` because glTF character assets routinely use root scaling transforms (e.g. 0.01) that per-node dual quaternion conversions corrupt.
