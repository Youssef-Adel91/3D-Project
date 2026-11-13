# Linear Blend Skinning (LBS) vs Dual Quaternion Skinning (DQS) GPU Benchmark Suite

This project implements an experimental evaluation and comparative benchmark of Linear Blend Skinning (LBS) and Dual Quaternion Skinning (DQS) in OpenGL 3.3. It benchmarks deformation quality (volume preservation, rotation-invariant cross-sectional thickness, and joint bulging across varying weight smoothness $W$), GPU vertex shader execution times across four architectures (LBS-naive, LBS-pre, DQS-naive, DQS-pre), bone palette upload mechanisms (Uniform Arrays, Uniform Buffer Objects, Texture Buffer Objects), and a production biped character (`Woman.gltf`). All measurements were collected directly on physical hardware using OpenGL timer queries (`GL_TIME_ELAPSED`) and high-resolution CPU performance counters without interpolation or synthetic smoothing.

---

## Attribution & License

- Base animation, math, and rendering routines are derived from:  
  **Hands-On C++ Game Animation Programming** by Gabor Szauer (Packt Publishing, 2020).  
  Repository: [https://github.com/gszauer/GameAnimationProgramming](https://github.com/gszauer/GameAnimationProgramming)  
  License: **MIT License** (preserved in [`LICENSE`](LICENSE)).

---

## Hardware & Driver Environment

All benchmark numbers reported were recorded on the following physical host system:

| Property | Value |
|---|---|
| **GPU** | NVIDIA T1200 Laptop GPU (4 GB GDDR6, PCIe) |
| **OpenGL Version** | 3.3.0 NVIDIA 610.60 (Core Profile) |
| **OpenGL Vendor** | NVIDIA Corporation |
| **Driver Version** | 610.60 |
| **CPU** | 11th Gen Intel(R) Core(TM) i7-11850H @ 2.50 GHz (8 cores, 16 threads) |
| **OS** | Windows 10 Pro (25H2) Build 26200 |
| **Vertical Sync (VSync)** | Disabled (`wglSwapIntervalEXT(0)`, confirmed `wglGetSwapIntervalEXT() == 0`) |
| **Uniform Limits** | `GL_MAX_VERTEX_UNIFORM_COMPONENTS`: 4,096 |
| **UBO Limits** | `GL_MAX_UNIFORM_BLOCK_SIZE`: 65,536 bytes (64 KB) |
| **TBO Limits** | `GL_MAX_TEXTURE_BUFFER_SIZE`: 268,435,456 texels |

Execution on the discrete GPU is forced via `NvOptimusEnablement = 0x00000001` and `AmdPowerXpressRequestHighPerformance = 1`.

---

## Build Instructions

Build x64 Release with a single MSBuild command using Visual Studio 2019 or 2022:

```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" Experiments\Sample.vcxproj /p:Configuration=Release /p:Platform=x64
Copy-Item Experiments\x64\Release\Sample.exe Experiments\Sample.exe -Force
```

---

## Command-Line Options & Output Files

Run from the `Experiments/` directory:

| Option | Purpose | Generated Output Files |
|---|---|---|
| `--e1` | Runs Experiment 1: Deformation quality, volume ratio, slice thickness on `dq.gltf`, transform-feedback GPU/CPU validation | `Results/e1_quality.csv`<br>`Results/e1_slices.csv` |
| `--e1b` | Runs Experiment 1b: Weight smoothness sensitivity sweep ($W \in [0.25, 8.0]$) and 3-joint chain | `Results/e1b_quality.csv`<br>`Results/e1b_three_joint.csv`<br>`Results/e1b_three_joint_v2.csv` |
| `--e2` | Runs Experiment 2: GPU timing sweep across vertex counts ($10\text{k}, 100\text{k}, 1\text{M}$), bones ($20, 60, 120$), influences ($1, 2, 4$), and Woman character | `Results/e2_gpu.csv` |
| `--e3` | Runs Experiment 3: Bone upload paths (Uniform, UBO, TBO) across bone counts ($60, 120, 256, 512$) | `Results/e3_path.csv` |
| `--e4` | Runs Experiment 4: Stronger LBS Baselines (LBS-pre-3x4, LBS-naive-opt, LBS-blend-once) and TF validation | `Results_v2/e4_lbs_baselines.csv`<br>`Results_v2/E4_REPORT.md` |
| `--compare-paths` | Control run comparing E2 and E4 harnesses on the identical 99,852-vertex mesh | `Results_v2/compare_paths.csv`<br>`Results_v2/smi_compare_paths.csv` |
| `--e5-exact` | Runs Experiment 5 at exact vertex counts (99,852 and 999,996) across coherent and shuffled orders | `Results_v2/e5_vertex_order_exact.csv` |
| `--e5b` | Runs Experiment 5b: Shuffled artifact verification (3 random seeds, block shuffle, vertex scaling) | `Results_v2/e5b_shuffle_checks.csv`<br>`Results_v2/E5b_REPORT.md` |
| `--e6` | Runs Experiment 6: Joint bulging and geometric quality limits across angles and ramp widths | `Results_v2/e6_quality.csv`<br>`Results_v2/e6_vertex_metric.csv`<br>`Results_v2/E6_REPORT.md` |
| `--e7` | Runs Experiment 7: Non-rigid scale and shear breakdown analysis | `Results_v2/e7_scale.csv`<br>`Results_v2/E7_REPORT.md` |
| `--e8` | Runs Experiment 8: Production character mesh at scale (`Woman.gltf` instanced at N=100 and N=1000) | `Results_v2/e8_woman.csv`<br>`Results_v2/E8_REPORT.md` |
| `--screenshots` | Automated capture of high-resolution split-view benchmark screenshots | `Screenshots_v2/*.png` |
| `--record [file]` | Records a 10-second animation loop of bar twist and character walk (MP4 / PNG frames) | `Screenshots_v2/*.mp4`<br>`Screenshots_v2/frames/*.png` |
| `--dead-code` | Measures discard mode vs full rendering where `fragPos` and `normal` are consumed | `Results_fixed/dead_code_bench.csv` |
| `--interactive` | Launches the interactive OpenGL split-view viewer (LBS on left, DQS on right) | Interactive display window |

---

## Reproducing the Paper

All tables and figures in the paper and the reviewer round can be reproduced from the generated CSV artifacts:

### Original Paper Tables & Figures
- **Table 1 (E1 Quality):** Generated by `--e1` $\to$ [`Experiments/Results/e1_quality.csv`](Experiments/Results/e1_quality.csv)
- **Table 2 & 3 (E1b Sensitivity & 3-Joint):** Generated by `--e1b` $\to$ [`Experiments/Results/e1b_quality.csv`](Experiments/Results/e1b_quality.csv), [`Experiments/Results/e1b_three_joint_v2.csv`](Experiments/Results/e1b_three_joint_v2.csv)
- **Tables 4–7 (E2 GPU Scaling):** Generated by `--e2` $\to$ [`Experiments/Results/e2_gpu.csv`](Experiments/Results/e2_gpu.csv)
- **Table 8 (E3 Upload Paths):** Generated by `--e3` $\to$ [`Experiments/Results/e3_path.csv`](Experiments/Results/e3_path.csv)
- **Table 9 (Production Character):** Generated by `--e2` $\to$ [`Experiments/Results/e2_gpu.csv`](Experiments/Results/e2_gpu.csv)

### Reviewer Round Tables & Artifacts
- **Table R1 (Control Run: E2 vs E4 Code Paths on Cool GPU):** Generated by `--compare-paths` $\to$ [`Experiments/Results_v2/compare_paths.csv`](Experiments/Results_v2/compare_paths.csv) and [`Experiments/Results_v2/smi_compare_paths.csv`](Experiments/Results_v2/smi_compare_paths.csv)
- **Table R2 (E4 Stronger LBS Baselines & FLOP Counts):** Generated by `--e4` $\to$ [`Experiments/Results_v2/e4_lbs_baselines.csv`](Experiments/Results_v2/e4_lbs_baselines.csv) and [`Experiments/Results_v2/E4_REPORT.md`](Experiments/Results_v2/E4_REPORT.md)
- **Table R3 (E5 Vertex Order at Exact Vertex Counts):** Generated by `--e5-exact` $\to$ [`Experiments/Results_v2/e5_vertex_order_exact.csv`](Experiments/Results_v2/e5_vertex_order_exact.csv)
- **Table R4 (E5b Shuffled Vertex Fetch Verification):** Generated by `--e5b` $\to$ [`Experiments/Results_v2/e5b_shuffle_checks.csv`](Experiments/Results_v2/e5b_shuffle_checks.csv) and [`Experiments/Results_v2/E5b_REPORT.md`](Experiments/Results_v2/E5b_REPORT.md)
- **Table R5 (E6 Joint Bulging & Metric Limits):** Generated by `--e6` $\to$ [`Experiments/Results_v2/e6_quality.csv`](Experiments/Results_v2/e6_quality.csv) and [`Experiments/Results_v2/e6_vertex_metric.csv`](Experiments/Results_v2/e6_vertex_metric.csv)
- **Table R6 (E7 Non-Rigid Scaling & Shear):** Generated by `--e7` $\to$ [`Experiments/Results_v2/e7_scale.csv`](Experiments/Results_v2/e7_scale.csv)
- **Table R7 (E8 Real-world Instancing at Scale):** Generated by `--e8` $\to$ [`Experiments/Results_v2/e8_woman.csv`](Experiments/Results_v2/e8_woman.csv)

To regenerate all summary tables and the corrected reviewer report:
```powershell
python Experiments/Results_v2/make_report_v2_tables.py
```
Output report is located at [`Experiments/Results_v2/REPORT_v2_corrected.md`](Experiments/Results_v2/REPORT_v2_corrected.md).

---

## Interactive Controls (`Sample.exe --interactive`)

The interactive mode displays side-by-side viewports comparing LBS (left) and DQS (right) with real-time FPS and joint angles in the window title:

- `[Space]`: Pause / Play continuous animation playback
- `[Left Arrow]`: Decrease twist or bend angle by 5° (when paused)
- `[Right Arrow]`: Increase twist or bend angle by 5° (when paused)
- `[B]`: Toggle model between Two-Joint Cylinder Bar and Woman walking character
- `[T]`: Toggle motion mode between Twist and Bend (on bar model)
- `[Esc]`: Quit application

---

## Paper Artifact Traceability Matrix

Every table and figure in the paper is mapped directly to the CSV or image artifact that generated it:

| Paper Item | Description | Source File | Key Metrics / Columns |
|---|---|---|---|
| **Table 1** | E1: Deformation Quality (`dq.gltf`) | [`Experiments/Results/e1_quality.csv`](Experiments/Results/e1_quality.csv) | `volume_ratio`, `min_thickness_ratio` (Twist 90/120/180°, Bend 90/120°) |
| **Table 2** | E1b: Weight Smoothness Sensitivity ($W$) | [`Experiments/Results/e1b_quality.csv`](Experiments/Results/e1b_quality.csv) | `volume_ratio`, `min_thickness` vs $W$, $1 - W/15$ formula |
| **Table 3** | E1b: Three-Joint Chain Evaluation | [`Experiments/Results/e1b_three_joint_v2.csv`](Experiments/Results/e1b_three_joint_v2.csv) | `volume_ratio`, `min_thickness`, `max_thickness` ($W \le 5.0$) |
| **Table 4** | E2: Vertex Count Scaling ($10\text{k}, 100\text{k}, 1\text{M}$) | [`Experiments/Results/e2_gpu.csv`](Experiments/Results/e2_gpu.csv) | `gpu_ms_median` [min–max] across 5 runs (60 bones, 4 influences) |
| **Table 5** | E2: Bone Count Scaling ($20, 43, 60, 120$) | [`Experiments/Results/e2_gpu.csv`](Experiments/Results/e2_gpu.csv) | `gpu_ms_median` across 5 runs (99,852 vertices, 4 influences) |
| **Table 6** | E2: Influence Count Scaling ($1, 2, 4$) | [`Experiments/Results/e2_gpu.csv`](Experiments/Results/e2_gpu.csv) | `gpu_ms_median` across 5 runs (99,852 vertices, 60 bones) |
| **Table 7** | E2: CPU Palette & Upload Times | [`Experiments/Results/e2_gpu.csv`](Experiments/Results/e2_gpu.csv) | `cpu_palette_ms`, `upload_ms` at 20, 60, 120 bones |
| **Table 8** | E3: Upload Paths & Driver Limits | [`Experiments/Results/e3_path.csv`](Experiments/Results/e3_path.csv) | `gpu_ms_median`, `status` (`ok` vs `exceeds_limit`) for Uniform, UBO, TBO |
| **Table 9** | Production Character (`Woman.gltf`) | [`Experiments/Results/e2_gpu.csv`](Experiments/Results/e2_gpu.csv) | `gpu_ms_median`, `cpu_palette_ms`, `upload_ms` on 41-joint character |
| **Figure 1** | Twist Volume Ratio vs Angle | [`Experiments/Results/plot1_twist_volume.png`](Experiments/Results/plot1_twist_volume.png) | LBS vs DQS vs Analytical curve (0° to 180°) |
| **Figure 2** | Radial Thickness Profile Along Joint Axis | [`Experiments/Results/plot2_twist_slices.png`](Experiments/Results/plot2_twist_slices.png) | Per-slice thickness at 30°, 90°, 150°, 180° |
| **Figure 3** | Bend Volume Retention & Bulging | [`Experiments/Results/plot3_bend_volume_bulge.png`](Experiments/Results/plot3_bend_volume_bulge.png) | Volume ratio and slice thickness (0° to 120°) |
| **Figure 4** | GPU Execution Time vs Vertex Count | [`Experiments/Results/plot4_gpu_vs_vertices.png`](Experiments/Results/plot4_gpu_vs_vertices.png) | Log-log scaling for all 4 variants up to 1M vertices |
| **Figure 5** | GPU Scaling by Bones and Influences | [`Experiments/Results/plot5_gpu_bones_influences.png`](Experiments/Results/plot5_gpu_bones_influences.png) | Performance sensitivity to skeletal complexity |
| **Figure 6** | Upload Paths vs Bone Count | [`Experiments/Results/plot6_upload_paths.png`](Experiments/Results/plot6_upload_paths.png) | Uniform array drop-off vs UBO vs TBO |
| **Figure 7** | Woman Production Character Breakdown | [`Experiments/Results/plot7_woman_comparison.png`](Experiments/Results/plot7_woman_comparison.png) | GPU, CPU palette, and upload breakdown |
| **Figure 8** | E1b Quality vs Weight Smoothness | [`Experiments/plot_e1b.png`](Experiments/plot_e1b.png) | 4-panel sensitivity plot vs ramp width $W$ |

All table values can be regenerated and verified in plain text by running:
```powershell
python Experiments/paper_tables.py
```
Output is logged to [`paper_tables_output.txt`](paper_tables_output.txt).

---

## Caveats & Known Limitations

1. **One GPU Architecture:** All benchmark measurements were collected on a single mobile workstation GPU: **NVIDIA T1200 Laptop GPU** (Turing TU117 architecture, 16 SMs, 1024 CUDA cores, 128-bit GDDR6 @ 10 Gbps, 160 GB/s nominal peak bandwidth). Hardware cache hierarchies, register counts, and memory buses on AMD RDNA or Intel Arc GPUs may exhibit different relative ALU vs bandwidth ratios.
2. **Thermal Throttling During Long Sweeps:** During the long, uninterrupted 27-configuration sweep (`--e2`, comprising over 1.2 million continuous frames without inter-test pauses), the GPU reached its target operating temperature of 73°C–75°C. At this temperature, the NVIDIA driver continuously asserted bitmask `0x0000000000000020` (`nvmlClocksThrottleReasonSwThermalSlowdown`), throttling the SM clock from its 2100 MHz boost peak down to 1200–1665 MHz. This elevated the measured runtime of DQS-pre at 99,852 vertices from 0.0696 ms (on a rested, cool GPU) to 0.0895 ms (under sustained thermal saturation). Isolated control runs (`--compare-paths`) confirm that on a rested, cool GPU, DQS-pre runs at 0.0696 ms.
3. **OpenGL 3.3 Core Profile:** Shaders use OpenGL 3.3 constructs (Uniform Arrays, UBO, TBO) with `GL_RASTERIZER_DISCARD`. Modern compute shader skinning and Shader Storage Buffer Objects (SSBOs, requiring OpenGL 4.3+) are not evaluated.
4. **Synthetic Meshes:** Benchmark meshes use procedural cylindrical geometry with uniform vertex distributions and linear blending ramps. Real-world character meshes with non-uniform vertex density and hand-painted weights may experience different texture and cache behaviors.
5. **Timer Query Resolution Floor:** Hardware timer queries (`GL_TIME_ELAPSED`) have an implementation precision limit of approximately 1–3 microseconds. For lightweight models ($< 10\text{k}$ vertices), measured execution times under 10 µs carry higher relative variance.
6. **DQS Non-Rigid Scale Limitation:** Dual Quaternion Skinning is mathematically defined for rigid body motions ($\text{SE}(3) = \text{SO}(3) \times \mathbb{R}^3$). DQS does not natively represent non-uniform bone scaling or affine shear; models requiring scale animations must decompose bone transforms into dual quaternions and separate scale matrices.
