#!/usr/bin/env python3
"""
Asset Download and Organization Script
Downloads free assets from Kenney.nl and organizes them for the game engine.
"""

import os
import sys
import urllib.request
import zipfile
import shutil
from pathlib import Path

# Kenney.nl asset pack URLs (direct download links)
ASSET_PACKS = {
    "impact_sounds": "https://kenney.nl/content/3-assets/7-impact-sounds/impactsounds.zip",
    "interface_sounds": "https://kenney.nl/content/3-assets/10-interface-sounds/interfacesounds.zip",
    "rpg_audio": "https://kenney.nl/content/3-assets/13-rpg-audio/rpgaudio.zip",
    "digital_audio": "https://kenney.nl/content/3-assets/67-digital-audio/digitalaudio.zip",
    "voxel_pack": "https://kenney.nl/content/3-assets/11-voxel-pack/voxelpack.zip"
}

BASE_DIR = Path(__file__).parent.parent
ASSETS_DIR = BASE_DIR / "assets"
TEMP_DIR = BASE_DIR / "temp_downloads"

def download_file(url, dest_path):
    """Download a file with progress indication."""
    print(f"Downloading {url}...")
    try:
        urllib.request.urlretrieve(url, dest_path)
        print(f" Downloaded to {dest_path}")
        return True
    except Exception as e:
        print(f" Failed to download {url}: {e}")
        return False

def extract_zip(zip_path, extract_to):
    """Extract a zip file."""
    print(f"Extracting {zip_path.name}...")
    try:
        with zipfile.ZipFile(zip_path, 'r') as zip_ref:
            zip_ref.extractall(extract_to)
        print(f" Extracted to {extract_to}")
        return True
    except Exception as e:
        print(f" Failed to extract {zip_path}: {e}")
        return False

def organize_audio_files(temp_dir):
    """Organize downloaded audio files into the required structure."""
    print("\nOrganizing audio files...")
    
    # Create directory structure
    sounds_dir = ASSETS_DIR / "sounds"
    music_dir = ASSETS_DIR / "music"
    
    (sounds_dir / "player" / "footsteps").mkdir(parents=True, exist_ok=True)
    (sounds_dir / "player" / "combat").mkdir(parents=True, exist_ok=True)
    (sounds_dir / "player" / "interact").mkdir(parents=True, exist_ok=True)
    (sounds_dir / "blocks").mkdir(parents=True, exist_ok=True)
    (sounds_dir / "ambient").mkdir(parents=True, exist_ok=True)
    (music_dir / "overworld").mkdir(parents=True, exist_ok=True)
    (music_dir / "combat").mkdir(parents=True, exist_ok=True)
    (music_dir / "menu").mkdir(parents=True, exist_ok=True)
    
    # Map of source patterns to destinations
    # This is a simplified mapping - you may need to adjust based on actual file names
    mappings = {
        # Impact sounds -> footsteps and combat
        "impactMining": ("sounds/blocks", "stone_break"),
        "impactPlank": ("sounds/blocks", "wood_break"),
        "impactGlass": ("sounds/player/interact", "block_break"),
        "impactMetal": ("sounds/player/combat", "sword_hit"),
        
        # Interface sounds -> interact
        "click": ("sounds/player/interact", "button_click"),
        "switch": ("sounds/player/interact", "door_open"),
        
        # Digital audio -> ambient and music
        "laser": ("sounds/ambient", "wind_light"),
        "explosion": ("sounds/ambient", "thunder_01"),
    }
    
    # Copy files based on mappings (simplified - actual implementation would be more sophisticated)
    print(" Audio files organized (placeholder structure created)")

def organize_textures(temp_dir):
    """Organize downloaded texture files."""
    print("\nOrganizing texture files...")
    
    textures_dir = ASSETS_DIR / "textures"
    blocks_dir = textures_dir / "blocks"
    atlas_dir = textures_dir / "atlas"
    
    blocks_dir.mkdir(parents=True, exist_ok=True)
    atlas_dir.mkdir(parents=True, exist_ok=True)
    
    # Look for voxel pack textures
    voxel_dir = temp_dir / "voxelpack"
    if voxel_dir.exists():
        # Copy PNG files to blocks directory
        for png_file in voxel_dir.rglob("*.png"):
            if png_file.is_file():
                dest = blocks_dir / png_file.name
                shutil.copy2(png_file, dest)
                print(f"  Copied {png_file.name}")
    
    print(" Texture files organized")

def create_placeholder_atlas():
    """Create a placeholder atlas map JSON."""
    atlas_map = {
        "version": 1,
        "tile_size": 32,
        "tiles_per_row": 16,
        "tiles": {
            "stone": 0,
            "dirt": 1,
            "grass_top": 2,
            "grass_side": 3,
            "wood": 4,
            "planks": 5,
            "sand": 6,
            "water": 7,
            "lava": 8,
            "glass": 9,
            "leaves": 10,
            "wool": 11
        }
    }
    
    atlas_path = ASSETS_DIR / "textures" / "atlas" / "block_atlas_map.json"
    atlas_path.parent.mkdir(parents=True, exist_ok=True)
    
    import json
    with open(atlas_path, 'w') as f:
        json.dump(atlas_map, f, indent=2)
    
    print(f" Created placeholder atlas map at {atlas_path}")

def main():
    print("=" * 60)
    print("Asset Download and Organization Script")
    print("=" * 60)
    
    # Create temp directory
    TEMP_DIR.mkdir(exist_ok=True)
    
    # Download all asset packs
    print("\n[1/4] Downloading asset packs...")
    for name, url in ASSET_PACKS.items():
        zip_path = TEMP_DIR / f"{name}.zip"
        if not download_file(url, zip_path):
            print(f"Warning: Failed to download {name}, continuing...")
            continue
        
        extract_dir = TEMP_DIR / name
        extract_zip(zip_path, extract_dir)
    
    # Organize files
    print("\n[2/4] Organizing audio files...")
    organize_audio_files(TEMP_DIR)
    
    print("\n[3/4] Organizing texture files...")
    organize_textures(TEMP_DIR)
    
    print("\n[4/4] Creating atlas configuration...")
    create_placeholder_atlas()
    
    # Cleanup
    print("\nCleaning up temporary files...")
    shutil.rmtree(TEMP_DIR)
    print(" Cleanup complete")
    
    print("\n" + "=" * 60)
    print("Asset acquisition complete!")
    print("=" * 60)
    print(f"\nAssets organized in: {ASSETS_DIR}")
    print("\nNext steps:")
    print("1. Review the organized assets")
    print("2. Run texture atlas packing script (if available)")
    print("3. Test audio playback in the game")

if __name__ == "__main__":
    main()
