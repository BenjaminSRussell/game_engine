#!/usr/bin/env python3
import json
import os
import sys
from pathlib import Path
from PIL import Image

def is_power_of_two(n):
    return n > 0 and (n & (n - 1)) == 0

def check_atlas_map(map_path):
    with map_path.open() as f:
        data = json.load(f)
    if data.get("tile_size") != 32:
        print(f"ERROR: tile_size must be 32, got {data.get('tile_size')}")
        return False
    tpr = data.get("tiles_per_row")
    if not is_power_of_two(tpr):
        print(f"ERROR: tiles_per_row must be power of two, got {tpr}")
        return False
    tiles = data.get("tiles", {})
    # Ensure no gaps up to max index
    max_idx = max(int(k) for k in tiles.keys()) if tiles else -1
    for i in range(max_idx + 1):
        if str(i) not in tiles:
            print(f"ERROR: atlas map missing entry for index {i}")
            return False
    print(f"OK: atlas map valid (tile_size={data['tile_size']}, tiles_per_row={tpr}, max_index={max_idx})")
    return True

def check_png_dimensions(blocks_dir):
    errors = []
    for png_path in blocks_dir.glob("*.png"):
        with Image.open(png_path) as img:
            if img.size != (32, 32):
                errors.append(f"{png_path.name} is {img.size[0]}x{img.size[1]}, expected 32x32")
    if errors:
        for e in errors:
            print(f"ERROR: {e}")
        return False
    else:
        print("OK: all PNGs are 32x32")
        return True

def check_atlas_image(atlas_path):
    if not atlas_path.exists():
        print("WARNING: atlas PNG not present")
        return True
    with Image.open(atlas_path) as img:
        w, h = img.size
        if not is_power_of_two(w) or not is_power_of_two(h):
            print(f"ERROR: atlas dimensions {w}x{h} not power-of-two")
            return False
        print(f"OK: atlas image {w}x{h} is power-of-two")
        return True

def main():
    root = Path(__file__).resolve().parents[1]
    atlas_map_path = root / "assets/textures/atlas/block_atlas_map.json"
    atlas_img_path = root / "assets/textures/atlas/block_atlas.png"
    blocks_dir = root / "assets/textures/blocks"

    ok = True
    if atlas_map_path.exists():
        ok &= check_atlas_map(atlas_map_path)
    else:
        print("WARNING: block_atlas_map.json not found")
    ok &= check_png_dimensions(blocks_dir)
    ok &= check_atlas_image(atlas_img_path)

    sys.exit(0 if ok else 1)

if __name__ == "__main__":
    main()
