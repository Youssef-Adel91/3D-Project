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

## 3. List of Generated Files

### A. Side-by-Side Screenshots (1280x720 PNG)
Each screenshot presents **LBS on the left** and **DQS on the right** separated by a vertical dividing line, with anti-aliased text badges ("LBS", "DQS", and deformation angle):

1. **[`bar_twist_0.png`](bar_twist_0.png)**: Cylinder bar in rest pose (0° twist). Both algorithms match the un-deformed geometry identically.
2. **[`bar_twist_90.png`](bar_twist_90.png)**: Cylinder bar at 90° twist. LBS exhibits noticeable volume loss and thinning at the center joint; DQS preserves circular cross-section.
3. **[`bar_twist_180.png`](bar_twist_180.png)**: Cylinder bar at 180° twist. Complete candy-wrapper collapse under LBS (thickness shrinks to ~39% of DQS); DQS maintains full cylindrical volume.
4. **[`bar_bend_0.png`](bar_bend_0.png)**: Cylinder bar in unbent rest pose (0° bend).
5. **[`bar_bend_60.png`](bar_bend_60.png)**: Cylinder bar bent at 60°. LBS shows interior joint flattening; DQS preserves joint profile.
6. **[`bar_bend_120.png`](bar_bend_120.png)**: Cylinder bar bent at 120° (extreme flexion). Severe joint pinching and interior surface collapse under LBS; DQS preserves exterior volume and smooth bulge.
7. **[`woman_walk.png`](woman_walk.png)**: Full-body framing of the Woman character during a walking stride ($t = 0.5\,\text{s}$). Frame encompasses head to toe with balanced vertical margins.

### B. Video Files (1280x720, 30 fps, H.264 High Profile, MP4)
1. **[`bar_twist.mp4`](bar_twist.mp4)**: 10-second smooth loop (300 frames) of the bar twisting smoothly from 0° to 180° and back to 0° using cosine ease.
2. **[`woman_walk.mp4`](woman_walk.mp4)**: 10-second continuous loop (300 frames) of the Woman character walking.

### C. Frame Sequences (`frames/`)
- Contains **600 numbered PNG frames** (1280x720):
  - `frames/bar_twist_0000.png` through `bar_twist_0299.png` (300 frames)
  - `frames/woman_walk_0000.png` through `woman_walk_0299.png` (300 frames)

---

## 4. Visual Inspection Checklist

Every generated output was opened, analyzed, and inspected:

1. **Lighting (`lit.frag`)**:
   - Replaced the single un-clamped directional light with a two-sided lighting model featuring key light (`vec3(0.5, 0.8, 0.7)`), soft fill light (`vec3(-0.5, 0.4, -0.6)`), an ambient floor of $0.38$, and front-facing normal correction (`gl_FrontFacing`).
   - Numerical verification across all 7 screenshots confirms **0 pixels below 15 luminance** on any model face. No black faces exist anywhere.
2. **Camera Framing**:
   - **Bar (`dq.gltf`)**: Oriented at a 3/4 elevated oblique angle (`eye = (7.5, 4.5, 9.0)`, `target = (0, 0, 0)`), providing a clear perspective of the central deformation zone.
   - **Woman (`Woman.gltf`)**: Calculated dynamic bounding box of the skinned mesh (`center = (0, 2.65, 0)`, height = 5.3 units, `camDist = 6.5`). Inspection confirms head top at $y = 50\,\text{px}$ (margin: 50 px) and feet bottom at $y = 669\,\text{px}$ (margin: 50 px). The entire body is visible without cutting off legs.
3. **Labels & Split View**:
   - Left viewport is clearly labeled with red badge **"LBS (Linear Blend)"**.
   - Right viewport is clearly labeled with green badge **"DQS (Dual Quaternion)"**.
   - Center badge reports deformation angle / motion state.
   - Bottom badges describe specific artifact characteristics.
