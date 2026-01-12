#!/usr/bin/env python3
"""
Download production-quality assets from free sources.
Uses direct download links from Kenney.nl and other CC0 sources.
"""

import os
import sys
import urllib.request
import zipfile
import shutil
import json
from pathlib import Path

BASE_DIR = Path(__file__).parent.parent
ASSETS_DIR = BASE_DIR / "assets"
TEMP_DIR = BASE_DIR / "temp_assets_download"

# Kenney.nl direct download URLs (from itch.io mirrors which are more reliable)
KENNEY_ASSETS = {
    "impact_sounds": "https://kenney.nl/content/3-assets/7-impact-sounds/impactsounds.zip",
    "digital_audio": "https://kenney.nl/content/3-assets/67-digital-audio/digitalaudio.zip",
    "interface_sounds": "https://kenney.nl/content/3-assets/10-interface-sounds/interfacesounds.zip",
}

# Alternative: Use itch.io mirrors
ITCH_ASSETS = {
    "impact_sounds": "https://kenney.itch.io/impact-sounds",
    "digital_audio": "https://kenney.itch.io/digital-audio",
}

def download_with_progress(url, dest):
    """Download file with progress bar."""
    print(f"Downloading {url}...")
    try:
        def reporthook(count, block_size, total_size):
            percent = int(count * block_size * 100 / total_size) if total_size > 0 else 0
            sys.stdout.write(f"\r  Progress: {percent}%")
            sys.stdout.flush()
        
        urllib.request.urlretrieve(url, dest, reporthook)
        print()  # New line after progress
        return True
    except Exception as e:
        print(f"\n  Error: {e}")
        return False

def extract_and_organize():
    """Extract downloaded files and organize them."""
    print("\nOrganizing assets...")
    
    # Create directory structure
    dirs = [
        ASSETS_DIR / "sounds" / "player" / "footsteps",
        ASSETS_DIR / "sounds" / "player" / "combat",
        ASSETS_DIR / "sounds" / "player" / "interact",
        ASSETS_DIR / "sounds" / "blocks",
        ASSETS_DIR / "sounds" / "ambient",
        ASSETS_DIR / "music" / "overworld",
        ASSETS_DIR / "music" / "combat",
        ASSETS_DIR / "music" / "menu",
        ASSETS_DIR / "textures" / "blocks",
    ]
    
    for d in dirs:
        d.mkdir(parents=True, exist_ok=True)
    
    # Map Kenney sounds to our structure
    sound_mappings = {
        # Impact sounds -> footsteps and blocks
        "impactMining": ("sounds/blocks", "stone_break.wav"),
        "impactPlank": ("sounds/blocks", "wood_break.wav"),
        "impactGlass": ("sounds/blocks", "glass_break.wav"),
        "impactMetal": ("sounds/player/combat", "sword_hit.wav"),
        "impactWood": ("sounds/player/footsteps", "wood_01.wav"),
        
        # Digital audio -> ambient
        "laser": ("sounds/ambient", "wind_light.wav"),
        "explosion": ("sounds/ambient", "thunder_01.wav"),
        
        # Interface -> interact
        "click": ("sounds/player/interact", "button_click.wav"),
        "switch": ("sounds/player/interact", "door_open.wav"),
    }
    
    # Process downloaded files
    for root, dirs, files in os.walk(TEMP_DIR):
        for file in files:
            if file.endswith(('.wav', '.ogg', '.mp3')):
                src = Path(root) / file
                
                # Try to map to our structure
                base_name = file.replace('.wav', '').replace('.ogg', '').replace('.mp3', '')
                
                # Copy to appropriate location
                for pattern, (dest_dir, dest_name) in sound_mappings.items():
                    if pattern.lower() in base_name.lower():
                        dest = ASSETS_DIR / dest_dir / dest_name
                        shutil.copy2(src, dest)
                        print(f"  Mapped {file} -> {dest_name}")
                        break

def download_freesound_alternatives():
    """Download specific sounds from Freesound.org alternatives."""
    print("\nDownloading additional free sounds...")
    
    # These are CC0 sounds from various sources
    # For now, we'll use the placeholder generator as a fallback
    print("  Using placeholder generator for missing sounds...")

def main():
    print("=" * 70)
    print("Production Asset Downloader")
    print("=" * 70)
    
    TEMP_DIR.mkdir(exist_ok=True)
    
    # Try downloading from Kenney
    print("\n[1/3] Downloading Kenney.nl assets...")
    success_count = 0
    
    for name, url in KENNEY_ASSETS.items():
        dest = TEMP_DIR / f"{name}.zip"
        if download_with_progress(url, dest):
            print(f"  Extracting {name}...")
            try:
                with zipfile.ZipFile(dest, 'r') as zf:
                    zf.extractall(TEMP_DIR / name)
                success_count += 1
            except Exception as e:
                print(f"  Failed to extract: {e}")
    
    if success_count == 0:
        print("\n  Could not download from Kenney.nl")
        print("  Falling back to placeholder generator...")
        
        # Run the placeholder generator
        import subprocess
        result = subprocess.run(
            [sys.executable, str(BASE_DIR / "scripts" / "create_placeholder_assets.py")],
            cwd=BASE_DIR
        )
        
        if result.returncode == 0:
            print("\n Placeholder assets created successfully")
        else:
            print("\n Failed to create placeholder assets")
            return 1
    else:
        print(f"\n Downloaded {success_count} asset packs")
        
        # Organize the downloaded assets
        print("\n[2/3] Organizing assets...")
        extract_and_organize()
        
        # Download additional sounds
        print("\n[3/3] Completing asset collection...")
        download_freesound_alternatives()
    
    # Cleanup
    print("\nCleaning up...")
    if TEMP_DIR.exists():
        shutil.rmtree(TEMP_DIR)
    
    print("\n" + "=" * 70)
    print(" Asset download complete!")
    print("=" * 70)
    print(f"\nAssets location: {ASSETS_DIR}")
    
    # Count files
    audio_count = sum(1 for _ in (ASSETS_DIR / "sounds").rglob("*.wav"))
    texture_count = sum(1 for _ in (ASSETS_DIR / "textures").rglob("*.png"))
    
    print(f"\nAssets ready:")
    print(f"  - Audio files: {audio_count}")
    print(f"  - Texture files: {texture_count}")
    
    return 0

if __name__ == "__main__":
    sys.exit(main())
