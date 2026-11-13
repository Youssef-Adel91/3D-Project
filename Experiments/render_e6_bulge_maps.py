import os
import sys
import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
from mpl_toolkits.mplot3d.art3d import Poly3DCollection
import matplotlib.colors as mcolors

def parse_ply(ply_path):
    verts = []
    colors = []
    faces = []
    
    with open(ply_path, 'r') as f:
        lines = f.readlines()
        
    header = True
    idx = 0
    num_verts = 0
    num_faces = 0
    
    while idx < len(lines):
        line = lines[idx].strip()
        idx += 1
        if line.startswith("element vertex"):
            num_verts = int(line.split()[-1])
        elif line.startswith("element face"):
            num_faces = int(line.split()[-1])
        elif line == "end_header":
            break
            
    for _ in range(num_verts):
        parts = lines[idx].strip().split()
        idx += 1
        x, y, z = float(parts[0]), float(parts[1]), float(parts[2])
        r, g, b = int(parts[3]) / 255.0, int(parts[4]) / 255.0, int(parts[5]) / 255.0
        verts.append([x, y, z])
        colors.append([r, g, b])
        
    for _ in range(num_faces):
        parts = lines[idx].strip().split()
        idx += 1
        # format: 3 i0 i1 i2
        i0, i1, i2 = int(parts[1]), int(parts[2]), int(parts[3])
        faces.append([i0, i1, i2])
        
    return np.array(verts), np.array(colors), np.array(faces)

def render_mesh(ply_path, title, out_png):
    if not os.path.exists(ply_path):
        print(f"File not found: {ply_path}")
        return
        
    verts, colors, faces = parse_ply(ply_path)
    
    fig = plt.figure(figsize=(10, 7), dpi=150)
    ax = fig.add_subplot(111, projection='3d')
    
    # Calculate face colors as mean of vertex colors
    face_colors = colors[faces].mean(axis=1)
    
    mesh_poly = Poly3DCollection(verts[faces], facecolors=face_colors, edgecolors='black', linewidths=0.2, alpha=0.95)
    ax.add_collection3d(mesh_poly)
    
    # Scale axes
    all_x = verts[:, 0]
    all_y = verts[:, 1]
    all_z = verts[:, 2]
    max_range = np.array([all_x.max()-all_x.min(), all_y.max()-all_y.min(), all_z.max()-all_z.min()]).max() / 2.0
    mid_x = (all_x.max()+all_x.min()) * 0.5
    mid_y = (all_y.max()+all_y.min()) * 0.5
    mid_z = (all_z.max()+all_z.min()) * 0.5
    ax.set_xlim(mid_x - max_range, mid_x + max_range)
    ax.set_ylim(mid_y - max_range, mid_y + max_range)
    ax.set_zlim(mid_z - max_range, mid_z + max_range)
    
    ax.view_init(elev=25, azim=45)
    ax.set_title(title, fontsize=13, fontweight='bold', pad=15)
    ax.set_xlabel('X')
    ax.set_ylabel('Y')
    ax.set_zlabel('Z')
    
    # Add custom colorbar: Blue (<1.0) -> White (=1.0) -> Red (>1.0)
    cmap = mcolors.LinearSegmentedColormap.from_list("bulge_map", ["#0055ff", "#ffffff", "#ff2200"])
    norm = mcolors.TwoSlopeNorm(vmin=0.5, vcenter=1.0, vmax=1.15)
    sm = plt.cm.ScalarMappable(cmap=cmap, norm=norm)
    sm.set_array([])
    cbar = fig.colorbar(sm, ax=ax, shrink=0.6, aspect=15, pad=0.1)
    cbar.set_label('Radial Ratio |p_def - c_def| / |p_rest - c_rest|\n(Blue: Collapse < 1.0, White: 1.0, Red: Bulge > 1.0)', fontsize=9)
    
    plt.tight_layout()
    plt.savefig(out_png, dpi=150)
    plt.close()
    print(f"Rendered {out_png}")

if __name__ == "__main__":
    render_mesh("Results_v2/e6_bar_lbs_150_w05.ply", "LBS Deformation (150°+150° Bend, W=0.5)\nCandy-Wrapper Collapse", "Results_v2/e6_bulge_map_lbs.png")
    render_mesh("Results_v2/e6_bar_dqs_150_w05.ply", "DQS Deformation (150°+150° Bend, W=0.5)\nVolume & Cross-Section Preserved (No Bulge)", "Results_v2/e6_bulge_map_dqs.png")
    if os.path.exists("Experiments/Results_v2"):
        render_mesh("Experiments/Results_v2/e6_bar_lbs_150_w05.ply", "LBS Deformation (150°+150° Bend, W=0.5)\nCandy-Wrapper Collapse", "Experiments/Results_v2/e6_bulge_map_lbs.png")
        render_mesh("Experiments/Results_v2/e6_bar_dqs_150_w05.ply", "DQS Deformation (150°+150° Bend, W=0.5)\nVolume & Cross-Section Preserved (No Bulge)", "Experiments/Results_v2/e6_bulge_map_dqs.png")

