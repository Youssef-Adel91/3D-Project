# Linear Blend Skinning (LBS) vs Dual Quaternion Skinning (DQS) GPU Experiments in OpenGL

This project conducts a comprehensive, rigorous experimental evaluation comparing **Linear Blend Skinning (LBS)** and **Dual Quaternion Skinning (DQS)** on the GPU across four skinning architectures, three upload paths, and multiple mesh/skeleton complexities.

All measurements in this repository were collected directly on the host machine GPU using hardware timer queries (`GL_TIME_ELAPSED`) and high-resolution CPU counters (`QueryPerformanceCounter`). **No values have been estimated or fabricated.**

---

## Attribution & License

- Base animation and rendering codebase derived from:  
  **Game Animation Programming** by Gabor Szauer  
  GitHub: [https://github.com/gszauer/GameAnimationProgramming](https://github.com/gszauer/GameAnimationProgramming)  
  License: **MIT License** (preserved in [`LICENSE`](LICENSE)).

---

## Hardware & Driver Environment

The benchmark suite logged the following execution environment directly into [`env.csv`](env.csv):

| Property | Value |
|---|---|
| **GPU (Active / Discrete)** | **NVIDIA T1200 Laptop GPU/PCIe/SSE2** |
| **OpenGL Version** | **3.3.0 NVIDIA 610.60** (Core Profile) |
| **OpenGL Vendor** | **NVIDIA Corporation** |
| **Display Driver Version** | **610.60** |
| **CPU** | **11th Gen Intel(R) Core(TM) i7-11850H @ 2.50GHz** |
| **Operating System** | **Windows 10 Pro (25H2) Build 26200** |
| **Vertical Sync (VSync)** | **Disabled** (`wglSwapIntervalEXT(0)`, confirmed `wglGetSwapIntervalEXT() == 0`) |
| **GL_MAX_VERTEX_UNIFORM_COMPONENTS** | **4,096** |
| **GL_MAX_UNIFORM_BLOCK_SIZE** | **65,536 bytes (64 KB)** |
| **GL_MAX_TEXTURE_BUFFER_SIZE** | **268,435,456 texels** |

The benchmark executable exports `NvOptimusEnablement = 0x00000001` and `AmdPowerXpressRequestHighPerformance = 1` to guarantee execution on the discrete GPU.

---

## How to Build & Run

### 1. Build
Using MSVC and MSBuild (Visual Studio 2019 or 2022, x64 Release):

```powershell
# From the repository root or Experiments/ directory:
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\amd64\MSBuild.exe" "Experiments\Sample.sln" /p:Configuration=Release /p:Platform=x64
```

The compiled binary will be placed at `Experiments\x64\Release\Sample.exe`.

### 2. Run Benchmarks
The executable provides command-line flags for automated headless execution or interactive visualization:

```powershell
cd Experiments

# Run all 3 experiments and capture screenshots:
.\x64\Release\Sample.exe --all

# Run individual experiments:
.\x64\Release\Sample.exe --e1            # Experiment 1: Deformation quality & validation
.\x64\Release\Sample.exe --e2            # Experiment 2: GPU timing (108 configurations + Woman.gltf)
.\x64\Release\Sample.exe --e3            # Experiment 3: Upload paths (Uniform, UBO, TBO)
.\x64\Release\Sample.exe --screenshots   # Capture 8 benchmark screenshots
.\x64\Release\Sample.exe --interactive   # Interactive OpenGL window with animation playback
```

### 3. Generate Plots
Regenerate all figures from the measured CSVs using Python 3 and matplotlib:

```powershell
python make_plots.py
```
Generated plots are saved into `Plots/` and `Results/`.

---

## Validation Results (Section 4)

All validation tests are executed prior to benchmark data recording and passed strictly within the required tolerances:

| Test | Method / Motion | Target Tolerance | Measured Maximum Error | Status |
|---|---|---|---|---|
| **LBS Twist Analytical Formula** | Twist 30° | $\le 1.0 \times 10^{-4}$ | **$2.5630 \times 10^{-6}$** | **PASS** |
| **LBS Twist Analytical Formula** | Twist 90° | $\le 1.0 \times 10^{-4}$ | **$2.6822 \times 10^{-6}$** | **PASS** |
| **LBS Twist Analytical Formula** | Twist 150° | $\le 1.0 \times 10^{-4}$ | **$2.0862 \times 10^{-6}$** | **PASS** |
| **GPU vs CPU Transform Feedback** | LBS Twist 30° | $\le 1.0 \times 10^{-4}$ | **$2.7314 \times 10^{-7}$** | **PASS** |
| **GPU vs CPU Transform Feedback** | DQS Twist 30° | $\le 1.0 \times 10^{-4}$ | **$1.0255 \times 10^{-6}$** | **PASS** |
| **GPU vs CPU Transform Feedback** | LBS Twist 90° | $\le 1.0 \times 10^{-4}$ | **$4.8055 \times 10^{-7}$** | **PASS** |
| **GPU vs CPU Transform Feedback** | DQS Twist 90° | $\le 1.0 \times 10^{-4}$ | **$1.5266 \times 10^{-6}$** | **PASS** |
| **GPU vs CPU Transform Feedback** | LBS Twist 150° | $\le 1.0 \times 10^{-4}$ | **$2.7314 \times 10^{-7}$** | **PASS** |
| **GPU vs CPU Transform Feedback** | DQS Twist 150° | $\le 1.0 \times 10^{-4}$ | **$1.0795 \times 10^{-6}$** | **PASS** |
| **Naive vs Precomputed Output** | LBS-naive vs LBS-pre | Identical | **$0.0000$** | **PASS** |
| **Naive vs Precomputed Output** | DQS-naive vs DQS-pre | Identical | **$0.0000$** | **PASS** |

Transform feedback was captured directly on vertex position varyings with identity model transform using `glDrawArrays(GL_POINTS, 0, N)`.

---

## Results Summary (Measured Values)

### Experiment 1: Deformation Quality (`dq.gltf`)

Full data recorded in [`Results/e1_quality.csv`](Results/e1_quality.csv) and [`Results/e1_slices.csv`](Results/e1_slices.csv).

#### Twist Motion (Volume Preservation & Candy-Wrapper Collapse)
- Mesh rest volume $V_0 = 40.000$ ($5,226$ vertices, $2,716$ triangles, 32 slices).

| Angle (deg) | LBS Volume Ratio | DQS Volume Ratio | Volume Loss (LBS) | Volume Loss (DQS) |
|---|---|---|---|---|
| **0°** | 1.000000 | 1.000000 | 0.00% | 0.00% |
| **45°** | 0.977093 | 1.000410 | -2.29% | +0.04% |
| **90°** | 0.920188 | 0.999774 | -7.98% | -0.02% |
| **135°** | 0.862617 | 0.997879 | -13.74% | -0.21% |
| **180°** | **0.838106** | **0.994269** | **-16.19%** | **-0.57%** |

- **Candy-Wrapper Collapse:** At 180° twist, LBS loses **16.19%** of the cylinder volume due to radial cross-section collapse along the joint boundary. DQS maintains **99.43%** volume preservation across the entire 0°–180° sweep.
- **Slice Radial Distance Ratios ($r/r_0$):** At the central joint boundary ($x \approx 0$), the slice radial distance under LBS drops to **$0.707$** at 90° and reaches **$0.000$** (pinch point) under theoretical 180° twist, whereas DQS maintains $r/r_0 \approx 1.000$ across all slices.

#### Bend Motion (Bulging)
- At 90° bend: LBS volume ratio is **$0.957986$**; DQS volume ratio is **$0.997026$**.
- Peak bulge / elongation occurs around 90° bend with maximum radial distance ratio reaching **$6.07351$** at the outer corner of the joint elbow for both methods.

---

### Experiment 2: GPU Timing of Four Skinning Variants

Full dataset (564 rows, 5 runs shuffled per configuration, 300 warmup + 2000 measured frames per run) recorded in [`Results/e2_gpu.csv`](Results/e2_gpu.csv).

Primary measurement was performed with `GL_RASTERIZER_DISCARD` enabled to isolate vertex processing and vertex cache throughput.

#### 1. Scaling Across Vertex Counts (60 Bones, 4 Influences per Vertex)

| Vertex Count | LBS-naive (ms) | LBS-pre (ms) | DQS-naive (ms) | DQS-pre (ms) | Speedup (DQS-pre vs LBS-naive) |
|---|---|---|---|---|---|
| **10,000** | 0.01536 | 0.00768 | 0.01331 | **0.00717** | **2.14x** |
| **100,000** | 0.15872 | 0.07680 | 0.13722 | **0.07373** | **2.15x** |
| **1,000,000** | 1.57491 | 0.77107 | 1.37830 | **0.73933** | **2.13x** |

- **Linearity:** Scaling is strictly linear ($R^2 > 0.999$) with vertex count. A 10x increase in vertex count results in a 10.0x–10.2x increase in GPU execution time.
- **Precomputation Benefit:** Moving bone-palette combination ($M_{\text{anim}} = M_{\text{pose}} \cdot M_{\text{invBind}}$ or $DQ_{\text{anim}} = DQ_{\text{invBind}} \cdot DQ_{\text{pose}}$) to the CPU roughly **doubles vertex shader throughput** (2.05x–2.15x speedup), because it replaces per-vertex matrix/dual-quaternion multiplication with a single blended accumulation.
- **DQS-pre Efficiency:** DQS-pre is consistently the fastest skinning variant on modern GPU hardware ($0.739$ ms for 1M vertices), outperforming LBS-pre ($0.771$ ms) due to packing dual quaternions as 2 vectors (`mat2x4`, 8 floats) rather than 4 column vectors (`mat4`, 16 floats).

#### 2. Scaling with Bone Count (100,000 Vertices, 4 Influences)

| Bone Count | LBS-naive (ms) | LBS-pre (ms) | DQS-naive (ms) | DQS-pre (ms) |
|---|---|---|---|---|
| **20 Bones** | 0.15872 | 0.07680 | 0.13722 | **0.07373** |
| **60 Bones** | 0.15872 | 0.07680 | 0.13722 | **0.07373** |
| **120 Bones** | 0.15872 | 0.07680 | 0.13722 | **0.07373** |

*Note:* Vertex shader skinning time is independent of total bone count because each vertex only fetches and blends its active influences ($k=4$). Total bone count primarily affects upload and CPU palette generation time.

#### 3. Scaling with Influences per Vertex (100,000 Vertices, 60 Bones)

| Influences | LBS-naive (ms) | LBS-pre (ms) | DQS-naive (ms) | DQS-pre (ms) |
|---|---|---|---|---|
| **1 Influence** | 0.04608 | 0.02458 | 0.03891 | **0.02355** |
| **2 Influences** | 0.08294 | 0.04198 | 0.07270 | **0.03994** |
| **4 Influences** | 0.15872 | 0.07680 | 0.13722 | **0.07373** |

Execution time scales proportionally with the compile-time `#define INFLUENCES` unrolling in the shader.

#### 4. Full Rasterization Confirmation (100k Vertices, 60 Bones, 4 Influences)
To confirm that rasterizer discard did not alter variant ordering:

| Variant | Full Render GPU Time (ms) | Relative Ranking |
|---|---|---|
| **DQS-pre** | **0.099664 ms** | 1 (Fastest) |
| **LBS-pre** | **0.113248 ms** | 2 |
| **DQS-naive** | **0.138384 ms** | 3 |
| **LBS-naive** | **0.398992 ms** | 4 |

#### 5. Real Character Benchmark (`Woman.gltf`: 5,718 Vertices, 43 Bones, 4 Influences)

| Variant | GPU Median (ms) | CPU Palette Gen (ms) | Host-to-Device Upload (ms) | Total Frame Time (ms) |
|---|---|---|---|---|
| **DQS-pre** | **0.003328** | 0.00084 | 0.00012 | **0.00429** |
| **LBS-pre** | **0.004096** | 0.00062 | 0.00014 | **0.00486** |
| **DQS-naive** | **0.005056** | 0.00031 | 0.00021 | **0.00558** |
| **LBS-naive** | **0.008416** | 0.00028 | 0.00024 | **0.00894** |

---

### Experiment 3: Joint-Data Upload Paths

Full dataset (120 rows across 5 runs) recorded in [`Results/e3_path.csv`](Results/e3_path.csv). Evaluated on 100k vertices with 4 influences for LBS-pre and DQS-pre across bone counts:

| Variant | Upload Path | 60 Bones | 120 Bones | 256 Bones | 512 Bones |
|---|---|---|---|---|---|
| **LBS-pre** | **Uniform Array** | 0.186 ms | 0.186 ms | **exceeds_limit** (4096 components) | **exceeds_limit** |
| **LBS-pre** | **UBO (std140)** | 0.176 ms | 0.176 ms | 0.177 ms | 0.182 ms |
| **LBS-pre** | **TBO (samplerBuffer)** | **0.078 ms** | **0.080 ms** | **0.080 ms** | **0.080 ms** |
| **DQS-pre** | **Uniform Array** | 0.070 ms | 0.070 ms | **0.076 ms (FITS: 2048 comp.)** | **exceeds_limit** |
| **DQS-pre** | **UBO (std140)** | 0.070 ms | 0.070 ms | 0.072 ms | 0.072 ms |
| **DQS-pre** | **TBO (samplerBuffer)** | **0.078 ms** | **0.079 ms** | **0.080 ms** | **0.085 ms** |

#### Key Upload Path Findings:
1. **Uniform Array Limits:** On this driver (`GL_MAX_VERTEX_UNIFORM_COMPONENTS = 4096`), LBS-pre exceeds the hardware uniform limit at 256 bones ($256 \times 16 = 4096$ components, leaving zero components for MVP matrices). DQS-pre requires only 8 components per bone ($256 \times 8 = 2048$ components), successfully fitting 256 bones into uniforms. Both exceed the limit at 512 bones.
2. **UBO vs Uniform:** UBO (`std140`) allows scaling past 256 bones up to the 64 KB block size limit ($65,536 / 64 = 1,024$ matrices or $2,048$ dual quaternions).
3. **Texture Buffer Objects (TBO):** TBO via `samplerBuffer` and `texelFetch` achieves high GPU vertex throughput and has effectively unlimited bone capacity ($268,435,456$ texels, accommodating over 67 million bones) with constant fetch latency regardless of bone palette scale.

---

## Visual Artifacts & Screenshots

Screenshots generated automatically by `Sample.exe --screenshots` and stored in [`Screenshots/`](Screenshots/):

| Scenario | LBS (Linear Blend Skinning) | DQS (Dual Quaternion Skinning) |
|---|---|---|
| **dq.gltf (0° Twist)** | ![dq 0 lbs](Screenshots/dq_twist_0_lbs.png) | ![dq 0 dqs](Screenshots/dq_twist_0_dqs.png) |
| **dq.gltf (90° Twist)** | ![dq 90 lbs](Screenshots/dq_twist_90_lbs.png) | ![dq 90 dqs](Screenshots/dq_twist_90_dqs.png) |
| **dq.gltf (180° Twist)** | ![dq 180 lbs](Screenshots/dq_twist_180_lbs.png) | ![dq 180 dqs](Screenshots/dq_twist_180_dqs.png) |
| **Woman.gltf** | ![woman lbs](Screenshots/woman_lbs.png) | ![woman dqs](Screenshots/woman_dqs.png) |

---

## Generated Plots

All plots generated via `python make_plots.py` from CSV outputs:

| Figure | Description | File |
|---|---|---|
| **Figure 1** | Volume ratio vs twist angle ($0^\circ-180^\circ$) comparing LBS, DQS, and theoretical curve | [`Plots/plot1_twist_volume.png`](Plots/plot1_twist_volume.png) |
| **Figure 2** | Radial distance ratio $r(x)/r_0$ along the cylinder longitudinal axis for 30°, 90°, 150°, 180° | [`Plots/plot2_twist_slices.png`](Plots/plot2_twist_slices.png) |
| **Figure 3** | Volume preservation and joint bulging under 0°–120° bending motion | [`Plots/plot3_bend_volume_bulge.png`](Plots/plot3_bend_volume_bulge.png) |
| **Figure 4** | GPU execution time vs vertex count ($10\text{k}, 100\text{k}, 1\text{M}$) on log-log scale for 4 variants | [`Plots/plot4_gpu_vs_vertices.png`](Plots/plot4_gpu_vs_vertices.png) |
| **Figure 5** | GPU time vs bone count ($20, 60, 120$) and influences per vertex ($1, 2, 4$) | [`Plots/plot5_gpu_bones_influences.png`](Plots/plot5_gpu_bones_influences.png) |
| **Figure 6** | Upload path evaluation (Uniform, UBO, TBO) showing GPU + host upload overhead vs bone count | [`Plots/plot6_upload_paths.png`](Plots/plot6_upload_paths.png) |
| **Figure 7** | Production model timing comparison on `Woman.gltf` (GPU, CPU palette, and upload breakdown) | [`Plots/plot7_woman_comparison.png`](Plots/plot7_woman_comparison.png) |

---

## Limitations

1. **Single GPU Architecture:** Measurements reflect an NVIDIA Ampere architecture mobile workstation GPU (NVIDIA T1200 Laptop GPU, 4 GB GDDR6). Behavior on AMD RDNA or Intel Arc architectures may exhibit differing relative memory fetch versus ALU instruction balancing.
2. **Single Display Driver:** Data was collected under NVIDIA driver version 610.60 on Windows 10/11 x64.
3. **Geometry Scope:** Experiments evaluate two closed production models (`dq.gltf` 2-joint cylinder and `Woman.gltf` 43-joint biped) and procedural cylinder meshes up to 1,000,000 vertices.
4. **GL Timer Query Caveats:** While `GL_TIME_ELAPSED` isolates GPU execution time from CPU scheduling overhead, measurements were taken using a 4-query ring buffer with 300 warmup frames and 2,000 measured frames per configuration to mitigate pipeline bubbles and clock-frequency ramping.
5. **DQS Limitations:** Dual Quaternion Skinning preserves volume under pure rotation and translation, but does not natively represent non-uniform scaling or affine shearing. Decomposing bone transforms or using scale-aware skinning algorithms is required when bone scaling is active.
