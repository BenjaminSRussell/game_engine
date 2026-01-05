#!/usr/bin/env python3
"""
Scan 3D rendering directories to identify missing and misplaced files
"""

import os
from pathlib import Path

BASE_DIR = "/Users/benjaminrussell/Desktop/Minecraft v2/src/engine/rendering/3d_rendering"

# Files mentioned in audit as misplaced (Phase 1.4)
PHASE1_4_FILES = {
    "dynamic_resolution.c": ["framebuffer/", "rendering/output/"],
    "render_target.c": ["framebuffer/", "rendering/output/"],
}

# Files mentioned as misplaced (Phase 1.5)
PHASE1_5_FILES = {
    "debug_lines.c": ["math/", "debugging/", "profiling/"],
    "debug_shapes.c": ["math/", "debugging/", "profiling/"],
    "frame_timer.c": ["math/", "statistics/", "profiling/"],
    "gpu_timer.c": ["math/", "statistics/", "profiling/"],
}

# Missing file (Phase 2.2)
PHASE2_2_FILES = {
    "continuous_lod.c": ["geometry/lod/", "geometry/streaming/"],
}

def find_file_anywhere(filename, base_dir):
    """Find file anywhere in tree"""
    for root, dirs, files in os.walk(base_dir):
        if filename in files:
            return os.path.join(root, filename)
    return None

def check_expected_locations(filename, expected_dirs, base_dir):
    """Check if file exists in any expected location"""
    for dir_path in expected_dirs:
        full_path = os.path.join(base_dir, dir_path, filename)
        if os.path.exists(full_path):
            return full_path, "FOUND_EXPECTED"
    
    # Not in expected location, search everywhere
    actual = find_file_anywhere(filename, base_dir)
    if actual:
        return actual, "MISPLACED"
    return None, "MISSING"

print("=" * 60)
print("3D RENDERING FILE AUDIT")
print("=" * 60)

all_files = {}
all_files.update(PHASE1_4_FILES)
all_files.update(PHASE1_5_FILES)
all_files.update(PHASE2_2_FILES)

missing = []
misplaced = []

for filename, expected_dirs in sorted(all_files.items()):
    location, status = check_expected_locations(filename, expected_dirs, BASE_DIR)
    
    if status == "MISSING":
        missing.append((filename, expected_dirs))
        print(f"\n✗ MISSING: {filename}")
        print(f"  Expected in: {', '.join(expected_dirs)}")
    elif status == "MISPLACED":
        rel_path = os.path.relpath(location, BASE_DIR)
        misplaced.append((filename, rel_path, expected_dirs))
        print(f"\n⚠ MISPLACED: {filename}")
        print(f"  Found at: {rel_path}")
        print(f"  Expected in: {', '.join(expected_dirs)}")
    else:
        rel_path = os.path.relpath(location, BASE_DIR)
        print(f"\n✓ OK: {filename}")
        print(f"  Location: {rel_path}")

print("\n" + "=" * 60)
print(f"SUMMARY: {len(missing)} missing, {len(misplaced)} misplaced")
print("=" * 60)
