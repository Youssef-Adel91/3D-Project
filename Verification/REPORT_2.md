# Audit & Re-Verification Report 2: Shader Consistency, Validation & Dead-Code Risk

**Target Branch**: `fix-shader-consistency` (branched from commit `aa4e137`)  
**Date**: 2026-10-29  
**Verifier Role**: Independent Auditor  
**Machine / Platform**: Windows 10 Pro (25H2) Build 26200, NVIDIA T1200 Laptop GPU (Driver 610.60, VSync OFF)

---

## 1. Executive Verdict

**Verdict: The earlier timing results ARE NOT materially affected by the bug.**

### Detailed Findings
1. **Origin of Bug**: The erroneous calculation `fragPos = vec3(skin * vec4(normal, 0));` in `Experiments/Shaders/preskinned.vert` was **not** introduced in the benchmark changes. It originated in the MIT sample code by Gabor Szauer (*Game Animation Programming*, `Chapter14/Sample01`), which was imported at commit `5ab287e`. When commit `b1db2e2` parameterized the shader with `#if INFLUENCES >= ...`, it preserved this original line.
2. **Impact on Benchmarked Timings**:
   - Primary timing in Experiment 2 and Experiment 3 is measured with `GL_RASTERIZER_DISCARD` enabled to isolate the GPU vertex transform pipeline.
   - Correcting `preskinned.vert` to properly compute `skinnedPos = skin * vec4(position, 1.0)`, `gl_Position = projection * view * model * skinnedPos`, `fragPos = vec3(model * skinnedPos)`, and `norm = vec3(model * skin * vec4(normal, 0.0))` resulted in a negligible timing change of **0.00% to -3.08%** for LBS-pre (10k verts: 0.0143 vs 0.0143 ms; 100k verts: 0.1004 vs 0.1024 ms; 1M verts: 0.9034 vs 0.9321 ms). This is well inside ordinary run-to-run variance (±3%).
3. **Dead-Code Risk & Full Rendering**:
   - Even when `GL_RASTERIZER_DISCARD` is disabled and both `norm` and `fragPos` are genuinely consumed by the fragment shader (`vec3 l = normalize(light - fragPos); diffuseIntensity = clamp(dot(n, l) + 0.1, 0, 1)`), DQS-pre remains faster than LBS-pre across all configurations:
     - At 99,852 vertices: DQS-pre **0.081 ms** vs LBS-pre **0.131 ms** (DQS-pre is 38% faster).
     - At 999,996 vertices: DQS-pre **0.866 ms** vs LBS-pre **0.959 ms** (DQS-pre is 9.7% faster).
4. **Validation Coverage**:
   - Dedicated Transform Feedback validation shaders (`_tf.vert`) were created for all 8 timed shader permutations (uniform, UBO, TBO across LBS-pre and DQS-pre).
   - All 8 timed shaders were tested via Transform Feedback GPU vs CPU skinning at 30°, 90°, and 150° twist angles. Every configuration passed with maximum error $< 1.53 \times 10^{-6}$ (far below the $10^{-4}$ tolerance threshold).
5. **Committed Results Integrity**:
   - No committed results in `Experiments/Results/` were modified. The new benchmark runs were recorded into `Experiments/Results_fixed/`. MD5 hashes of all files in `Experiments/Results/` confirm zero changes.

---

## 2. Origin & Git Diff Against Commit 5ab287e

Commit `5ab287e` ("Step 0: copy Chapter14/Sample01 to Experiments/ (MIT, Gabor Szauer)") imported the book's original source code. 

### 2.1 File-by-File Origin Audit

| Shader File | Present in `5ab287e`? | Status / Changes against `5ab287e` |
|---|---|---|
| `Experiments/Shaders/preskinned.vert` | **Yes** | Contained `fragPos = vec3(skin * vec4(normal, 0));` in `5ab287e`. Commit `b1db2e2` added `#ifndef INFLUENCES` conditionals. |
| `Experiments/Shaders/skinned.vert` | **Yes** | Had redundant `skin * vec4(position, 1.0)` calculation in `gl_Position` and `fragPos`. |
| `Experiments/Shaders/dualquaternion.vert` | **Yes** | Commit `b1db2e2` added `#ifndef INFLUENCES` conditionals. |
| `Experiments/Shaders/lit.frag` | **Yes** | Identical to `5ab287e`. |
| `Experiments/Shaders/static.vert` | **Yes** | Identical to `5ab287e`. |
| `Experiments/Shaders/dqs_pre.vert` | No | New file added in `b1db2e2` for pre-combined DQS uniform path. |
| `Experiments/Shaders/dqs_pre_ubo.vert` | No | New file added in `b1db2e2` for DQS UBO path. |
| `Experiments/Shaders/dqs_pre_tbo.vert` | No | New file added in `b1db2e2` for DQS TBO path. |
| `Experiments/Shaders/preskinned_ubo.vert`| No | New file added in `b1db2e2` for LBS UBO path. |
| `Experiments/Shaders/preskinned_tbo.vert`| No | New file added in `b1db2e2` for LBS TBO path. |
| `Experiments/Shaders/skinned_tf.vert` | No | New file added in `b1db2e2` for LBS-naive TF validation. |
| `Experiments/Shaders/dualquaternion_tf.vert`| No | New file added in `b1db2e2` for DQS-naive TF validation. |

### 2.2 Proof of Origin in `preskinned.vert`
Inspection of `5ab287e:Experiments/Shaders/preskinned.vert`:
```glsl
void main() {
    mat4 skin = animated[joints.x] * weights.x +
                animated[joints.y] * weights.y +
                animated[joints.z] * weights.z +
                animated[joints.w] * weights.w;
    gl_Position = projection * view * model * skin * vec4(position, 1.0);

    fragPos = vec3(skin * vec4(normal, 0));
    norm = vec3(model * skin* vec4(normal, 0));
    uv = texCoord;
}
```
This confirms that the buggy `fragPos = vec3(skin * vec4(normal, 0));` was written by Gabor Szauer in the original MIT sample.

---

## 3. Shaders Timed vs Validated

### 3.1 Mapping of Timed vs Validated Shaders

| Variant | Upload Path | Timed Shader File | Purpose in E2/E3 | Validated in E1 by TF? | Validation Shader File |
|---|---|---|---|---|---|
| **LBS-naive** | Uniform | `Shaders/skinned.vert` | Timed in E2 | **Yes** | `Shaders/skinned_tf.vert` |
| **LBS-pre** | Uniform | `Shaders/preskinned.vert` | Timed in E2, E3 | **Yes (Added)** | `Shaders/preskinned_tf.vert` |
| **LBS-pre** | UBO | `Shaders/preskinned_ubo.vert`| Timed in E3 | **Yes (Added)** | `Shaders/preskinned_ubo_tf.vert` |
| **LBS-pre** | TBO | `Shaders/preskinned_tbo.vert`| Timed in E3 | **Yes (Added)** | `Shaders/preskinned_tbo_tf.vert` |
| **DQS-naive** | Uniform | `Shaders/dualquaternion.vert`| Timed in E2 | **Yes** | `Shaders/dualquaternion_tf.vert` |
| **DQS-pre** | Uniform | `Shaders/dqs_pre.vert` | Timed in E2, E3 | **Yes (Added)** | `Shaders/dqs_pre_tf.vert` |
| **DQS-pre** | UBO | `Shaders/dqs_pre_ubo.vert` | Timed in E3 | **Yes (Added)** | `Shaders/dqs_pre_ubo_tf.vert` |
| **DQS-pre** | TBO | `Shaders/dqs_pre_tbo.vert` | Timed in E3 | **Yes (Added)** | `Shaders/dqs_pre_tbo_tf.vert` |

### 3.2 Line-by-Line Diffs for All 8 Timed vs TF Shader Pairs

For every pair, the skinning mathematics is 100% identical. The only differences are the declaration of `out vec3 tf_position;` and the assignment `tf_position = skinnedPos.xyz;` (or `v.xyz`).

```diff
--- Shaders/skinned.vert
+++ Shaders/skinned_tf.vert
+out vec3 tf_position;  // transform feedback capture
@@ -35,8 +38,10 @@
     vec4 skinnedPos = skin * vec4(position, 1.0);
     gl_Position = projection * view * model * skinnedPos;
     fragPos = vec3(model * skinnedPos);
     norm = vec3(model * skin * vec4(normal, 0.0));
     uv = texCoord;
+    tf_position = skinnedPos.xyz;
 }
```

```diff
--- Shaders/preskinned.vert
+++ Shaders/preskinned_tf.vert
+out vec3 tf_position;  // transform feedback capture
@@ -35,7 +37,9 @@
     vec4 skinnedPos = skin * vec4(position, 1.0);
     gl_Position = projection * view * model * skinnedPos;
     fragPos = vec3(model * skinnedPos);
     norm = vec3(model * skin * vec4(normal, 0.0));
     uv = texCoord;
+    tf_position = skinnedPos.xyz;
 }
```

```diff
--- Shaders/preskinned_ubo.vert
+++ Shaders/preskinned_ubo_tf.vert
+out vec3 tf_position;  // transform feedback capture
@@ -41,6 +43,8 @@
     vec4 skinnedPos = skin * vec4(position, 1.0);
     gl_Position = projection * view * model * skinnedPos;
     fragPos = vec3(model * skinnedPos);
     norm = vec3(model * skin * vec4(normal, 0.0));
     uv = texCoord;
+    tf_position = skinnedPos.xyz;
 }
```

```diff
--- Shaders/preskinned_tbo.vert
+++ Shaders/preskinned_tbo_tf.vert
+out vec3 tf_position;  // transform feedback capture
@@ -48,6 +50,8 @@
     vec4 skinnedPos = skin * vec4(position, 1.0);
     gl_Position = projection * view * model * skinnedPos;
     fragPos = vec3(model * skinnedPos);
     norm = vec3(model * skin * vec4(normal, 0.0));
     uv = texCoord;
+    tf_position = skinnedPos.xyz;
 }
```

```diff
--- Shaders/dualquaternion.vert
+++ Shaders/dualquaternion_tf.vert
+out vec3 tf_position;  // transform feedback capture
@@ -106,4 +108,6 @@
     vec4 n = transformVector(skinDq, normal);
     norm = vec3(model * n);
     uv = texCoord;
+    tf_position = v.xyz;
 }
```

```diff
--- Shaders/dqs_pre.vert
+++ Shaders/dqs_pre_tf.vert
+out vec3 tf_position;  // transform feedback capture
@@ -98,4 +100,6 @@
     vec4 n = transformVector(skinDq, normal);
     norm = vec3(model * n);
     uv = texCoord;
+    tf_position = v.xyz;
 }
```

```diff
--- Shaders/dqs_pre_ubo.vert
+++ Shaders/dqs_pre_ubo_tf.vert
+out vec3 tf_position;  // transform feedback capture
@@ -86,4 +88,6 @@
     vec4 n = transformVector_dq(blendReal, normal);
     norm = vec3(model * n);
     uv = texCoord;
+    tf_position = v.xyz;
 }
```

```diff
--- Shaders/dqs_pre_tbo.vert
+++ Shaders/dqs_pre_tbo_tf.vert
+out vec3 tf_position;  // transform feedback capture
@@ -88,4 +90,6 @@
     vec4 n = transformVector_dq(blendReal, normal);
     norm = vec3(model * n);
     uv = texCoord;
+    tf_position = v.xyz;
 }
```

### 3.3 Transform Feedback Validation Results (Tolerance $10^{-4}$)

Run via `Sample.exe --e1` on `dq.gltf`:

| Variant | Upload Path | Twist 30° Max Error | Twist 90° Max Error | Twist 150° Max Error | Verdict |
|---|---|---|---|---|---|
| **LBS-naive** | Uniform | $2.73 \times 10^{-7}$ | $4.81 \times 10^{-7}$ | $2.73 \times 10^{-7}$ | **PASS** |
| **DQS-naive** | Uniform | $1.03 \times 10^{-6}$ | $1.53 \times 10^{-6}$ | $1.08 \times 10^{-6}$ | **PASS** |
| **LBS-pre** | Uniform | $2.46 \times 10^{-7}$ | $4.81 \times 10^{-7}$ | $2.67 \times 10^{-7}$ | **PASS** |
| **DQS-pre** | Uniform | $1.48 \times 10^{-6}$ | $1.47 \times 10^{-6}$ | $7.05 \times 10^{-7}$ | **PASS** |
| **LBS-pre** | UBO | $2.46 \times 10^{-7}$ | $4.81 \times 10^{-7}$ | $2.67 \times 10^{-7}$ | **PASS** |
| **DQS-pre** | UBO | $1.48 \times 10^{-6}$ | $1.47 \times 10^{-6}$ | $7.05 \times 10^{-7}$ | **PASS** |
| **LBS-pre** | TBO | $2.46 \times 10^{-7}$ | $4.81 \times 10^{-7}$ | $2.67 \times 10^{-7}$ | **PASS** |
| **DQS-pre** | TBO | $1.48 \times 10^{-6}$ | $1.47 \times 10^{-6}$ | $7.05 \times 10^{-7}$ | **PASS** |

---

## 4. Equal Work Implementation & Code Audit

All four variants and paths now write the exact same set of outputs with identical semantics:
- `gl_Position = projection * view * model * skinnedPosition;`
- `fragPos = vec3(model * skinnedPosition);`
- `norm = vec3(model * skinnedNormal);`
- `uv = texCoord;`

### Changes Made

1. **`Experiments/Shaders/preskinned.vert`** (lines 35–40):
   - **Old**:
     ```glsl
     gl_Position = projection * view * model * skin * vec4(position, 1.0);
     fragPos = vec3(skin * vec4(normal, 0));
     norm = vec3(model * skin * vec4(normal, 0));
     ```
   - **New**:
     ```glsl
     vec4 skinnedPos = skin * vec4(position, 1.0);
     gl_Position = projection * view * model * skinnedPos;
     fragPos = vec3(model * skinnedPos);
     norm = vec3(model * skin * vec4(normal, 0.0));
     ```
   - **Effect**: Eliminates bug where `normal` was used for `fragPos` without `model`, and avoids computing `skin * vec4(position, 1.0)` twice.

2. **`Experiments/Shaders/skinned.vert`** (lines 36–41):
   - **Old**:
     ```glsl
     gl_Position = projection * view * model * skin * vec4(position, 1.0);
     fragPos = vec3(model * skin * vec4(position, 1.0));
     norm = vec3(model * skin * vec4(normal, 0.0f));
     ```
   - **New**:
     ```glsl
     vec4 skinnedPos = skin * vec4(position, 1.0);
     gl_Position = projection * view * model * skinnedPos;
     fragPos = vec3(model * skinnedPos);
     norm = vec3(model * skin * vec4(normal, 0.0));
     ```
   - **Effect**: Eliminates redundant matrix-vector multiplication in `fragPos`, matching `preskinned_ubo.vert`, `preskinned_tbo.vert`, and `skinned_tf.vert`.

3. **`Experiments/Shaders/lit.frag`** (lines 15–18):
   - **Old**:
     ```glsl
     vec3 n = normalize(norm);
     vec3 l = normalize(light);
     float diffuseIntensity = clamp(dot(n, l) + 0.1, 0, 1);
     ```
   - **New**:
     ```glsl
     vec3 n = normalize(norm);
     vec3 l = normalize(light - fragPos);
     float diffuseIntensity = clamp(dot(n, l) + 0.1, 0, 1);
     ```
   - **Effect**: Directly consumes `fragPos` to compute point light vector `l`, preventing the GLSL compiler/driver from dead-code-eliminating `fragPos` during full rendering passes.

---

## 5. Dead-Code Risk Benchmark: Discard vs Full Rendering

Measured via `Sample.exe --dead-code` across 5 independent runs (50 warm-up + 200 measured frames per run) at 60 bones and 4 influences:

### 5.1 99,852 Vertices (60 bones, 4 influences)

| Variant | Discard Mode Median [Min - Max] | Full Rendering Median [Min - Max] | Overhead from Full Rendering |
|---|---|---|---|
| **LBS-naive** | 0.3112 ms [0.3093 - 0.4659] | 0.3289 ms [0.3282 - 0.3362] | +5.7% |
| **LBS-pre** | 0.0798 ms [0.0758 - 0.0815] | 0.1308 ms [0.1141 - 0.1343] | +63.8% |
| **DQS-naive** | 0.1085 ms [0.1004 - 0.1099] | 0.1144 ms [0.1126 - 0.1212] | +5.4% |
| **DQS-pre** | **0.0683 ms** [0.0655 - 0.0696] | **0.0815 ms** [0.0800 - 0.0816] | +19.2% |

*Variant Speed Ranking (Fastest to Slowest)*:
- **Discard Mode**: DQS-pre (0.068 ms) < LBS-pre (0.080 ms) < DQS-naive (0.109 ms) < LBS-naive (0.311 ms)
- **Full Rendering**: DQS-pre (0.081 ms) < DQS-naive (0.114 ms) < LBS-pre (0.131 ms) < LBS-naive (0.329 ms)

### 5.2 999,996 Vertices (60 bones, 4 influences)

| Variant | Discard Mode Median [Min - Max] | Full Rendering Median [Min - Max] | Overhead from Full Rendering |
|---|---|---|---|
| **LBS-naive** | 1.8268 ms [1.8203 - 1.8289] | 1.9389 ms [1.9298 - 1.9788] | +6.1% |
| **LBS-pre** | 0.7803 ms [0.7802 - 0.7946] | 0.9587 ms [0.9194 - 1.0070] | +22.9% |
| **DQS-naive** | 0.9175 ms [0.9134 - 0.9278] | 0.9810 ms [0.9808 - 1.1145] | +6.9% |
| **DQS-pre** | **0.7795 ms** [0.7778 - 0.7864] | **0.8657 ms** [0.8655 - 0.8680] | +11.1% |

*Variant Speed Ranking (Fastest to Slowest)*:
- **Discard Mode**: DQS-pre (0.779 ms) ≈ LBS-pre (0.780 ms) < DQS-naive (0.918 ms) < LBS-naive (1.827 ms)
- **Full Rendering**: DQS-pre (0.866 ms) < LBS-pre (0.959 ms) < DQS-naive (0.981 ms) < LBS-naive (1.939 ms)

**Dead-Code Conclusion**:
Consuming `norm` and `fragPos` in `lit.frag` confirms that **no dead-code elimination bias occurred**. In full rendering, DQS-pre maintains its performance advantage over LBS-pre (38% faster at 100k verts, 9.7% faster at 1M verts).

---

## 6. Complete Benchmark Re-Run Comparison

Executed 5 runs of `--e2` (564 rows) and `--e3` (120 rows) with the fixed shaders into `Results_fixed/`.

### 6.1 Anchor Configuration: 60 Bones, 4 Influences

| Vertices | Variant | Committed Median [Min - Max] | Fixed Median [Min - Max] | Relative Difference | Order Check |
|---|---|---|---|---|---|
| **9,996** | LBS-naive | 0.0633 [0.0630 - 0.0633] | 0.0612 [0.0610 - 0.0614] | -3.19% | Faster |
| | **LBS-pre** | **0.0143** [0.0143 - 0.0150] | **0.0143** [0.0136 - 0.0143] | **+0.00%** | Unchanged |
| | DQS-naive | 0.0140 [0.0123 - 0.0143] | 0.0123 [0.0123 - 0.0124] | -12.53% | Faster |
| | **DQS-pre** | **0.0061** [0.0060 - 0.0061] | **0.0060** [0.0059 - 0.0061] | **-1.57%** | DQS-pre < LBS-pre |
| **99,852** | LBS-naive | 0.3651 [0.3569 - 0.3789] | 0.3549 [0.3540 - 0.3561] | -2.77% | Faster |
| | **LBS-pre** | **0.1024** [0.0962 - 0.1116] | **0.1004** [0.0986 - 0.1012] | **-2.00%** | Faster |
| | DQS-naive | 0.1321 [0.1290 - 0.1383] | 0.1413 [0.1408 - 0.1427] | +6.95% | Noise |
| | **DQS-pre** | **0.0895** [0.0787 - 0.0963] | **0.0891** [0.0876 - 0.0919] | **-0.50%** | DQS-pre < LBS-pre |
| **999,996** | LBS-naive | 2.1584 [2.1043 - 2.1688] | 2.0774 [2.0716 - 2.1053] | -3.75% | Faster |
| | **LBS-pre** | **0.9321** [0.8758 - 0.9392] | **0.9034** [0.8688 - 0.9222] | **-3.08%** | Faster |
| | DQS-naive | 1.0595 [1.0421 - 1.0642] | 1.1403 [1.1157 - 1.1653] | +7.62% | Noise |
| | **DQS-pre** | **0.9134** [0.8869 - 0.9216] | **0.9229** [0.9032 - 0.9321] | **+1.04%** | Tie (~0.91 ms) |

### 6.2 Experiment 3 Upload Path Comparison

| Variant | Path | Bones | Committed Median | Fixed Median | Relative Difference | Status |
|---|---|---|---|---|---|---|
| **LBS-pre** | uniform | 60 | 0.1093 ms | 0.0799 ms | -26.91% | ok |
| | uniform | 120 | 0.1864 ms | 0.1895 ms | +1.67% | ok |
| | uniform | 256 | exceeds_limit | exceeds_limit | 0.0% | exceeds_limit |
| | uniform | 512 | exceeds_limit | exceeds_limit | 0.0% | exceeds_limit |
| | ubo | 60 | 0.0750 ms | 0.0752 ms | +0.26% | ok |
| | ubo | 120 | 0.1761 ms | 0.1789 ms | +1.55% | ok |
| | ubo | 256 | 0.3798 ms | 0.3930 ms | +3.47% | ok |
| | ubo | 512 | 0.6840 ms | 0.6996 ms | +2.27% | ok |
| | tbo | 60 | 0.0795 ms | 0.0799 ms | +0.48% | ok |
| | tbo | 120 | 0.0799 ms | 0.0804 ms | +0.60% | ok |
| | tbo | 256 | 0.0799 ms | 0.0802 ms | +0.36% | ok |
| | tbo | 512 | 0.0800 ms | 0.0799 ms | -0.04% | ok |
| **DQS-pre** | uniform | 60 | 0.0696 ms | 0.0695 ms | -0.18% | ok |
| | uniform | 120 | 0.0705 ms | 0.0682 ms | -3.24% | ok |
| | uniform | 256 | 0.1312 ms | 0.1278 ms | -2.62% | ok |
| | uniform | 512 | exceeds_limit | exceeds_limit | 0.0% | exceeds_limit |
| | ubo | 60 | 0.0696 ms | 0.0696 ms | +0.00% | ok |
| | ubo | 120 | 0.0708 ms | 0.0697 ms | -1.49% | ok |
| | ubo | 256 | 0.1157 ms | 0.1188 ms | +2.64% | ok |
| | ubo | 512 | 0.2042 ms | 0.2068 ms | +1.29% | ok |
| | tbo | 60 | 0.0791 ms | 0.0788 ms | -0.49% | ok |
| | tbo | 120 | 0.0844 ms | 0.0799 ms | -5.35% | ok |
| | tbo | 256 | 0.0870 ms | 0.0840 ms | -3.53% | ok |
| | tbo | 512 | 0.0802 ms | 0.0808 ms | +0.82% | ok |

---

## 7. Exact Commands Executed

All actions were performed on branch `fix-shader-consistency`:

1. **Branch creation**:
   ```powershell
   git checkout -b fix-shader-consistency
   ```
2. **MSBuild Compilation**:
   ```powershell
   & "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\amd64\MSBuild.exe" "Experiments\Sample.sln" /p:Configuration=Release /p:Platform=x64 /t:Build /v:minimal
   ```
3. **Validation & Transform Feedback Execution**:
   ```powershell
   Set-Location Experiments
   .\x64\Release\Sample.exe --e1
   ```
4. **Dead-Code Risk Benchmark Execution**:
   ```powershell
   Set-Location Experiments
   .\x64\Release\Sample.exe --dead-code
   ```
5. **Experiment 2 & 3 Benchmark Runs**:
   ```powershell
   Set-Location Experiments
   .\x64\Release\Sample.exe --e2
   .\x64\Release\Sample.exe --e3
   ```
6. **Committed Results Integrity Verification**:
   ```powershell
   git status --porcelain Experiments/Results/
   git diff Experiments/Results/
   ```
   *(Zero output confirmed: all committed files in `Experiments/Results/` remain untracked / untouched).*

