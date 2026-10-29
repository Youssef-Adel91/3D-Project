"""
Post-processes screenshots and renders video files for LBS vs DQS visual presentation.
- Overlays clean, anti-aliased labels ("LBS", "DQS", and state title) onto split views.
- Saves high-quality PNGs in Screenshots_v2/.
- Encodes 10-second MP4 loops (Screenshots_v2/bar_twist.mp4 and woman_walk.mp4).
- Saves numbered PNG frames in Screenshots_v2/frames/.
"""

import os
import sys
import glob
import shutil
import subprocess
from PIL import Image, ImageDraw, ImageFont

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SCREENSHOTS_DIR = os.path.join(SCRIPT_DIR, "Screenshots_v2")
FRAMES_DIR = os.path.join(SCREENSHOTS_DIR, "frames")

os.makedirs(SCREENSHOTS_DIR, exist_ok=True)
os.makedirs(FRAMES_DIR, exist_ok=True)

def get_font(size):
    font_names = [
        "segoeui.ttf", "arial.ttf", "calibri.ttf", "tahoma.ttf",
        "C:\\Windows\\Fonts\\segoeuib.ttf", "C:\\Windows\\Fonts\\segoeui.ttf",
        "C:\\Windows\\Fonts\\arialbd.ttf", "C:\\Windows\\Fonts\\arial.ttf"
    ]
    for name in font_names:
        try:
            return ImageFont.truetype(name, size)
        except Exception:
            continue
    return ImageFont.load_default()

def draw_pill(draw, x, y, text, font, bg_color=(20, 24, 32, 210), fg_color=(255, 255, 255, 255), pad_x=14, pad_y=6):
    bbox = font.getbbox(text)
    w = bbox[2] - bbox[0]
    h = bbox[3] - bbox[1]
    rect = [x, y, x + w + pad_x * 2, y + h + pad_y * 2]
    r = (rect[3] - rect[1]) // 2
    draw.rounded_rectangle(rect, radius=r, fill=bg_color, outline=(255, 255, 255, 60), width=1)
    text_x = x + pad_x - bbox[0]
    text_y = y + pad_y - bbox[1]
    draw.text((text_x, text_y), text, font=font, fill=fg_color)
    return rect[2] - rect[0]

def annotate_image(img_path, title_text, out_png_path):
    im = Image.open(img_path).convert("RGBA")
    w, h = im.size
    half_w = w // 2

    overlay = Image.new("RGBA", im.size, (0, 0, 0, 0))
    draw = ImageDraw.Draw(overlay)

    # 1. Subtle vertical separator line between the two viewports
    draw.line([(half_w, 0), (half_w, h)], fill=(255, 255, 255, 80), width=2)

    font_label = get_font(22)
    font_sub = get_font(15)
    font_title = get_font(20)

    # 2. Left label: "LBS (Linear Blend)"
    draw_pill(draw, 24, 22, "LBS (Linear Blend)", font_label, bg_color=(220, 48, 48, 220))

    # 3. Right label: "DQS (Dual Quaternion)"
    draw_pill(draw, half_w + 24, 22, "DQS (Dual Quaternion)", font_label, bg_color=(34, 139, 34, 220))

    # 4. Center Title Badge
    bbox = font_title.getbbox(title_text)
    tw = bbox[2] - bbox[0]
    th = bbox[3] - bbox[1]
    pill_w = tw + 32
    pill_h = th + 14
    cx = (w - pill_w) // 2
    draw.rounded_rectangle([cx, 22, cx + pill_w, 22 + pill_h], radius=pill_h//2,
                           fill=(15, 18, 24, 230), outline=(255, 255, 255, 90), width=1)
    draw.text((cx + 16 - bbox[0], 22 + 7 - bbox[1]), title_text, font=font_title, fill=(255, 255, 255, 255))

    # 5. Bottom comparison notes
    lbs_note = "Candy-wrapper volume loss" if "Twist" in title_text else "Joint collapse / pinching" if "Bend" in title_text else "Classical linear blending"
    dqs_note = "Volume preserved (rigid rotation)" if "Twist" in title_text else "Bulging preserved" if "Bend" in title_text else "Dual quaternion blending"

    draw_pill(draw, 24, h - 46, lbs_note, font_sub, bg_color=(10, 12, 16, 190), fg_color=(230, 230, 230, 230), pad_x=10, pad_y=4)
    draw_pill(draw, half_w + 24, h - 46, dqs_note, font_sub, bg_color=(10, 12, 16, 190), fg_color=(230, 230, 230, 230), pad_x=10, pad_y=4)

    final_img = Image.alpha_composite(im, overlay).convert("RGB")
    final_img.save(out_png_path, "PNG", optimize=True)
    print(f"[Annotated] {out_png_path}")

def annotate_screenshots():
    shots = [
        ("raw_bar_twist_0.bmp",   "Bar Twist: 0° (Rest Pose)",                       "bar_twist_0.png"),
        ("raw_bar_twist_90.bmp",  "Bar Twist: 90° (50% Twist - Onset of Thinning)",  "bar_twist_90.png"),
        ("raw_bar_twist_180.bmp", "Bar Twist: 180° (Full Candy-Wrapper Collapse)",   "bar_twist_180.png"),
        ("raw_bar_bend_0.bmp",    "Bar Bend: 0° (Rest Pose)",                        "bar_bend_0.png"),
        ("raw_bar_bend_60.bmp",   "Bar Bend: 60° (Joint Flexion)",                   "bar_bend_60.png"),
        ("raw_bar_bend_120.bmp",  "Bar Bend: 120° (Extreme Flexion)",                "bar_bend_120.png"),
        ("raw_woman_walk.bmp",    "Woman Character (Walking Pose - Full Body Frame)","woman_walk.png")
    ]

    for raw_name, title, png_name in shots:
        raw_path = os.path.join(SCREENSHOTS_DIR, raw_name)
        png_path = os.path.join(SCREENSHOTS_DIR, png_name)
        if os.path.exists(raw_path):
            annotate_image(raw_path, title, png_path)
        else:
            print(f"[Warning] Raw file not found: {raw_path}")

def annotate_video_frame(b_path, title_prefix, current_str):
    im = Image.open(b_path).convert("RGBA")
    w, h = im.size
    half_w = w // 2

    overlay = Image.new("RGBA", im.size, (0, 0, 0, 0))
    draw = ImageDraw.Draw(overlay)

    draw.line([(half_w, 0), (half_w, h)], fill=(255, 255, 255, 80), width=2)

    font_label = get_font(20)
    font_title = get_font(18)

    draw_pill(draw, 24, 20, "LBS (Linear Blend)", font_label, bg_color=(220, 48, 48, 220))
    draw_pill(draw, half_w + 24, 20, "DQS (Dual Quaternion)", font_label, bg_color=(34, 139, 34, 220))

    title_text = f"{title_prefix}: {current_str}"
    bbox = font_title.getbbox(title_text)
    tw = bbox[2] - bbox[0]
    th = bbox[3] - bbox[1]
    pill_w = tw + 28
    pill_h = th + 12
    cx = (w - pill_w) // 2
    draw.rounded_rectangle([cx, 20, cx + pill_w, 20 + pill_h], radius=pill_h//2,
                           fill=(15, 18, 24, 230), outline=(255, 255, 255, 90), width=1)
    draw.text((cx + 14 - bbox[0], 20 + 6 - bbox[1]), title_text, font=font_title, fill=(255, 255, 255, 255))

    final_img = Image.alpha_composite(im, overlay).convert("RGB")
    png_path = os.path.splitext(b_path)[0] + ".png"
    final_img.save(png_path, "PNG")

def process_frames_to_png():
    import math
    print("[Frames] Processing and labeling frames in Screenshots_v2/frames/ ...")
    
    # 1. Bar twist frames
    bar_bmps = sorted(glob.glob(os.path.join(FRAMES_DIR, "bar_twist_*.bmp")))
    total_bar = len(bar_bmps)
    for i, b in enumerate(bar_bmps):
        t = i / float(total_bar) if total_bar > 0 else 0.0
        angle = 180.0 * 0.5 * (1.0 - math.cos(t * 2.0 * math.pi))
        annotate_video_frame(b, "Bar Twist Loop", f"{angle:.1f}°")

    # 2. Woman walk frames
    woman_bmps = sorted(glob.glob(os.path.join(FRAMES_DIR, "woman_walk_*.bmp")))
    for i, b in enumerate(woman_bmps):
        t_sec = i / 30.0
        annotate_video_frame(b, "Woman Walking", f"{t_sec:.2f}s")

    print(f"[Frames] Converted {len(bar_bmps)} bar frames and {len(woman_bmps)} woman frames to labeled PNGs.")

def encode_mp4(record_out=None):
    process_frames_to_png()

    ffmpeg_exe = "ffmpeg"
    try:
        subprocess.run(["ffmpeg", "-version"], stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True)
    except Exception:
        python_scripts = os.path.join(os.path.dirname(sys.executable), "Scripts", "ffmpeg.exe")
        if os.path.exists(python_scripts):
            ffmpeg_exe = python_scripts
        else:
            print("[Warning] ffmpeg not found. Keeping PNG frames only.")
            return

    # 1. Bar Twist MP4
    bar_pattern = os.path.join(FRAMES_DIR, "bar_twist_%04d.png")
    bar_mp4 = os.path.join(SCREENSHOTS_DIR, "bar_twist.mp4")
    if glob.glob(os.path.join(FRAMES_DIR, "bar_twist_*.png")):
        cmd = [
            ffmpeg_exe, "-y",
            "-framerate", "30",
            "-i", bar_pattern,
            "-c:v", "libx264",
            "-profile:v", "high",
            "-crf", "18",
            "-pix_fmt", "yuv420p",
            bar_mp4
        ]
        print("[FFmpeg] Encoding:", " ".join(cmd))
        subprocess.run(cmd, check=True)
        print(f"[FFmpeg] Successfully generated: {bar_mp4}")

    # 2. Woman Walk MP4
    woman_pattern = os.path.join(FRAMES_DIR, "woman_walk_%04d.png")
    woman_mp4 = os.path.join(SCREENSHOTS_DIR, "woman_walk.mp4")
    if glob.glob(os.path.join(FRAMES_DIR, "woman_walk_*.png")):
        cmd = [
            ffmpeg_exe, "-y",
            "-framerate", "30",
            "-i", woman_pattern,
            "-c:v", "libx264",
            "-profile:v", "high",
            "-crf", "18",
            "-pix_fmt", "yuv420p",
            woman_mp4
        ]
        print("[FFmpeg] Encoding:", " ".join(cmd))
        subprocess.run(cmd, check=True)
        print(f"[FFmpeg] Successfully generated: {woman_mp4}")

    if record_out and record_out != "Screenshots_v2/out.mp4":
        target = os.path.abspath(record_out)
        shutil.copyfile(bar_mp4, target)
        print(f"[FFmpeg] Copied output to {target}")

if __name__ == "__main__":
    mode = sys.argv[1] if len(sys.argv) > 1 else "--all"
    if mode == "--annotate" or mode == "--all":
        annotate_screenshots()
    if mode == "--frames":
        process_frames_to_png()
    if mode == "--encode" or mode == "--all":
        rec = sys.argv[2] if len(sys.argv) > 2 else None
        encode_mp4(rec)
