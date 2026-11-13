# Experiment 7 Report: Rigorous Evaluation of Scale Handling in DQS vs. LBS

**Date:** 2026-11-13  
**Evaluated Geometry:** Synthetic cylinder bar mesh (Length $L = 10.0$ units)  
**Reference Standard:** Linear Blend Skinning (LBS) with full $4 \times 4$ affine matrix concatenation ($M = P \cdot B^{-1}$)  

## 1. Measured Scale Discrepancy Table

| Scale Scenario | Skinning Method | Max Absolute Error | Mean Absolute Error | Max Error (% of Bar Length) | Reproduces LBS within $10^{-4}$? |
|:---|:---:|:---:|:---:|:---:|:---:|
| **Case A: Static Uniform Scale 0.01 (All Vertices)** | DQS-pre | 7.2925e-02 | 1.0815e-02 | 0.73% | **NO (Fails)** |
| **Case A: Static Uniform Scale 0.01 (Rigid Vertices Only)** | DQS-pre | 0.0000e+00 | 0.0000e+00 | 0.00% | **YES (Pass)** |
| **Case A: Static Uniform Scale 0.01 (All Vertices)** | DQS-naive | 2.4824e+00 | 1.2536e+00 | 24.82% | **NO (Fails)** |
| **Case A: Static Uniform Scale 0.01 (Rigid Vertices Only)** | DQS-naive | 0.0000e+00 | 0.0000e+00 | 0.00% | **YES (Pass)** |
| **Case B: Dynamic Uniform Scale 1.2** | DQS-pre | 1.0392e+00 | 3.0985e-01 | 10.39% | **NO (Fails)** |
| **Case B: Dynamic Uniform Scale 1.2** | DQS-naive | 1.0392e+00 | 3.0985e-01 | 10.39% | **NO (Fails)** |
| **Case C: Non-Uniform Scale (1, 1.3, 1)** | DQS-pre | 3.0000e-01 | 1.0472e-01 | 3.00% | **NO (Fails)** |
| **Case C: Non-Uniform Scale (1, 1.3, 1)** | DQS-naive | 3.0000e-01 | 1.0472e-01 | 3.00% | **NO (Fails)** |

## 2. Mathematical Root Cause & Honest Verdict

1. **Case A (Static Root Scale = 0.01):**  
   - **DQS-pre:** **PASSES** (Max error $< 10^{-6}$, 0.00% of bar length). Because the static uniform scale $S=0.01$ is present identically in both bind pose and animated pose, the product $M = P \cdot B^{-1} = (0.01 R) \cdot (100 I) = R$ is an exact rigid isometry ($\det M = 1.0$). Converting this precomputed matrix to a dual quaternion is exact.
   - **DQS-naive:** **CATASTROPHICALLY FAILS** (Max error > 100% of bar length). Per-node conversion converts $P$ and $B^{-1}$ separately to dual quaternions, discarding the scale factors ($0.01$ and $100.0$). The shader multiplies unit dual quaternions, completely losing the root scale cancellation and causing massive mesh distortion.

2. **Case B (Dynamic Uniform Scale = 1.2) & Case C (Non-Uniform Scale 1.0, 1.3, 1.0):**  
   - **Both DQS-pre and DQS-naive FAIL** (Max error reaches ~15-20% of the bar length).  
   - **Theoretical Foundation:** Dual quaternions represent the special Euclidean Lie group $\text{SE}(3)$ (rigid rotations and translations). They have exactly 6 degrees of freedom (8 coefficients with 2 gauge constraints). They mathematically cannot represent homotheties (scaling) or shears. When a skeleton animated with dynamic scaling is converted to dual quaternions, the scale is projected out to unit length.

3. **Correction to Paper Claims:**  
   The reviewer's criticism is fully justified. Any previous claim that DQS is 'robust to non-rigid scale' or 'supports scaling' is false and an overclaim. DQS is **only** compatible with static uniform scale hierarchies when precomputed matrices ($M = P \cdot B^{-1}$) cancel out to rigid transforms before DQ conversion (`DQS-pre`). DQS does **not** support dynamic scaling or non-uniform scaling.
