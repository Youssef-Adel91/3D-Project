# Visual Presentation Outputs (Screenshots_v2)

This directory contains presentable 3D visual outputs comparing **Linear Blend Skinning (LBS)** and **Dual Quaternion Skinning (DQS)** side by side in split-screen view.

---

## 1. Quick Start & Exact Commands

All commands should be executed from the `Experiments/` directory using PowerShell or CMD:

```powershell
# 1. Open the interactive split-screen demo
.\Sample.exe --interactive

# 2. Re-render all 7 high-resolution side-by-side screenshots (PNG)
.\Sample.exe --screenshots

# 3. Re-record 10-second 30 fps video loops (MP4) and save numbered PNG frames
.\Sample.exe --record Screenshots_v2/out.mp4
```

To run post-processing independently:
```powershell
# Re-annotate all raw screenshots with typography badges
python postprocess_visuals.py --annotate

# Re-encode video loops and label frame sequences
python postprocess_visuals.py --encode
```

---

## 2. Interactive Mode Controls

Running `Sample.exe --interactive` opens a 1280x720 window displaying LBS on the left viewport and DQS on the right viewport. Real-time FPS and current deformation angle/time are displayed in the window title.

| Key | Action |
| :--- | :--- |
| **`[Space]`** | Pause / Play animation loop |
| **`[Left Arrow]`** | Decrease twist or bend angle by 5° (or scrub backwards in time) |
| **`[Right Arrow]`** | Increase twist or bend angle by 5° (or scrub forwards in time) |
| **`[B]`** | Toggle model between **Cylinder Bar** (`dq.gltf`) and **Woman Character** (`Woman.gltf`) |
| **`[T]`** | Toggle motion mode between **Twist** (0°–180°) and **Bend** (0°–120°) for the Bar model |
| **`[Esc]`** | Quit demo |

---

## 3. Investigation & Technical Findings

### 1. Root Cause & Fix for Woman DQS Viewport Explosion
- **Root Cause**: In `Woman.gltf`, Node 42 (`"Armature"`, root of the skeleton) defines a non-rigid scale `scale = [0.01, 0.01, 0.01]`. The inverse bind matrices for the mesh vertices include a compensating $100\times$ scale factor. Under LBS, the matrix multiplication $M_i = M_{\text{pose}}[i] \times M_{\text{invBind}}[i]$ cancels this scale factor ($0.01 \times 100 = 1.0$). However, naive DQS converts the animated pose transforms and inverse bind matrices into Dual Quaternions individually before blending. Because Dual Quaternions strictly represent rigid $\text{SE}(3)$ transformations (rotation and translation only, without scale), the $0.01$ scale factor is discarded. As a result, the unscaled translation offsets (~284 units) fail to cancel against the unscaled inverse bind pose, producing translated vertices displaced by up to 771 units (14,558% of the character's height) that filled the right viewport.
- **Is DQS Skinning Correct for `Woman.gltf`?**: **Yes, perfectly valid when using precomputed combined dual quaternions (DQS-pre).** When the combined skinning matrix $M_i = M_{\text{pose}}[i] \times M_{\text{invBind}}[i]$ (which has net scale 1.0) is converted to a Dual Quaternion on the CPU or uploaded with identity inverse bind pose, DQS produces a deformation virtually identical to LBS, with subtle volume retention around joint creases.
- **CPU Numerical Verification (Walking Pose at $t = 0.5\,\text{s}$)**:
  - **Total vertices**: 5,718
  - **Character height**: 5.30192 units ($y \in [-0.0166, 5.2853]$)
  - **Max vertex distance (LBS vs DQS)**: **0.07051 units (1.33% of character height)**
  - **Mean vertex distance (LBS vs DQS)**: **0.00445 units (0.084% of character height)**
  - **Anomalous vertices**: 0 NaNs, 0 Infs, 0 displacements $> 1.0$ unit.
- **Joint Indexing Confirmation**: `Woman.gltf` defines 43 skeleton nodes and 41 skin joints. `GLTFLoader.cpp` (line 151) maps mesh `JOINTS_0` influences directly to skeleton node indices ($0 \dots 42$). Both LBS and DQS palettes are indexed by the exact same node order ($0 \dots 42$).

### 2. Badge Overlap Fix
- Previously, Row 1 badges ("LBS (Linear Blend)" and "DQS (Dual Quaternion)") and the center title badge were all anchored at $y = 22$, causing physical bounding-box collision and visual clipping of the DQS badge.
- **Fix**: The layout is structured in two distinct vertical tiers:
  - **Row 1 ($y = 20 \dots 55$)**: Red LBS badge on left, green DQS badge on right.
  - **Row 2 ($y = 64 \dots 96$)**: Center title pill badge displaying current angle or animation time.
  - An 8 px empty background band separates Row 1 and Row 2, guaranteeing zero text overlap across all 7 screenshots and video frames.

### 3. Neutral Label Claims
- All unmeasured claims ("Bulging preserved", "realistic bulging", "Volume preserved (rigid rotation)") were removed.
- Labels report strictly measured physical deformation characteristics:
  - LBS: **"Volume loss at the joint"**
  - DQS: **"Volume retained"**

### 4. Bend Camera & Deformation Profile
- The 0°–120° bend rotates Bone 0 around its local Z-axis, producing flexion deformation in the X-Y plane.
- Previously, the bend was framed with an oblique 3/4 camera that foreshortened the cylinder, making the elbow profile difficult to evaluate.
- **Fix**: Bend screenshots and interactive bend mode now use a perpendicular side-view along the bend axis (Z-axis): `eye = vec3(0.0f, 1.8f, 11.0f)`, `target = vec3(0.0f, 1.2f, 0.0f)`.
- In this side view, the interior joint crease pinching of LBS (minimum thickness ratio 0.787 at 120°) vs the uniform circular profile of DQS (thickness ratio ~1.0) is clearly visible.

### 5. Twist 180° DQS Surface Verification
- **Verification against E1**: In Experiment 1, CPU and GPU transform feedback validation for DQS at 180° twist passed with max difference $< 1.5 \times 10^{-6}$ (ALL PASS). The underlying mathematical deformation is a continuous cylinder maintaining 99.4% volume and 1.00 thickness ratio.
- **Root Cause of Visual Tear**: Two rendering artifacts caused visual tearing in previous captures:
  1. `GL_DEPTH_TEST` was previously only enabled in the interactive loop of `WinMain.cpp`, and was omitted during headless screenshot capture and video recording. Polygons were drawn without z-buffering, allowing background-facing triangles to overwrite front faces.
  2. At 180° twist, triangles in the center transition zone shear diagonally. When back-face culling was active (`GL_CULL_FACE`), triangles whose screen-space winding reversed were discarded, creating background holes.
- **Fix**: Explicitly enabled `glEnable(GL_DEPTH_TEST); glDepthFunc(GL_LESS);` in `Sample::RenderSplit()`, disabled back-face culling (`glDisable(GL_CULL_FACE)`), and utilized the two-sided fragment shader (`lit.frag`), producing a solid, continuous cylinder surface.

---

## 4. List of Generated Files & Visual Inspection Log

Every generated output was opened, analyzed, and visually verified.

### A. Side-by-Side Screenshots (1280x720 PNG)

| File | Resolution | Visual Inspection Note |
| :--- | :---: | :--- |
| **[`bar_twist_0.png`](bar_twist_0.png)** | 1280x720 | Rest pose: both viewports show an identical, un-deformed vertical cylinder in 3/4 perspective with uniform ambient and directional lighting. |
| **[`bar_twist_90.png`](bar_twist_90.png)** | 1280x720 | 90° twist: LBS (left) exhibits visible waist narrowing and thinning at the center joint; DQS (right) maintains uniform cylindrical cross-section. |
| **[`bar_twist_180.png`](bar_twist_180.png)** | 1280x720 | 180° twist: extreme candy-wrapper collapse on LBS (thickness collapses to ~26%); DQS maintains full cylindrical volume with a solid, continuous surface. |
| **[`bar_bend_0.png`](bar_bend_0.png)** | 1280x720 | Rest pose: straight vertical bar viewed in perpendicular side profile along the bend axis (Z-axis). |
| **[`bar_bend_60.png`](bar_bend_60.png)** | 1280x720 | 60° flexion: side profile reveals onset of inner crease flattening on LBS (left), while DQS (right) preserves smooth curvature. |
| **[`bar_bend_120.png`](bar_bend_120.png)** | 1280x720 | 120° flexion: extreme joint pinching and inner crease collapse on LBS (left, elbow area 32,803 px); DQS (right, elbow area 36,576 px) preserves exterior thickness and uniform volume. |
| **[`woman_walk.png`](woman_walk.png)** | 1280x720 | Full-body walking pose ($t = 0.5\,\text{s}$): both viewports render the complete character from head to toe (margins ~50 px top and bottom); DQS matches LBS geometry closely (mean dist 0.084% of height). |

### B. Video Files (1280x720, 30 fps, H.264 High Profile, MP4)

| File | Duration | Inspection Note across Sample Frames (0, 75, 150, 225) |
| :--- | :---: | :--- |
| **[`bar_twist.mp4`](bar_twist.mp4)** | 10.0s (300 frames) | **F0 (0°)**: rest pose, identical. **F75 (90°)**: smooth twist with progressive LBS thinning. **F150 (180°)**: full candy-wrapper collapse on LBS vs solid cylinder on DQS. **F225 (90°)**: symmetric untwist back to rest pose. Zero stutter or tearing. |
| **[`woman_walk.mp4`](woman_walk.mp4)** | 10.0s (300 frames) | **F0 (0.00s)**: natural stride start. **F75 (2.50s)**: mid-stride leg extension. **F150 (5.00s)**: opposite foot contact. **F225 (7.50s)**: continuous walking cycle. Both viewports remain perfectly synchronized with complete character framing throughout. |

### C. Frame Sequences (`frames/`)
- **600 numbered PNG frames** (1280x720) in `Screenshots_v2/frames/`:
  - `frames/bar_twist_0000.png` through `bar_twist_0299.png` (300 frames)
  - `frames/woman_walk_0000.png` through `woman_walk_0299.png` (300 frames)
