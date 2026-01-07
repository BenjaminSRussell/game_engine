#!/usr/bin/env python3
"""
Download and replace placeholder assets with better quality free assets.
Uses alternative sources like OpenGameArt, Freesound, and direct GitHub repos.
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
TEMP_DIR = BASE_DIR / "temp_asset_replacement"

# Free asset sources with working direct links
ASSET_SOURCES = {
    # Minecraft-style textures from faithful pack (GitHub)
    "faithful_textures": {
        "url": "https://github.com/Faithful-Resource-Pack/Faithful-Java-32x/archive/refs/heads/main.zip",
        "type": "textures"
    },
    
    # Sound effects from various CC0 sources
    "freesound_pack": {
        # We'll use individual file downloads
        "type": "audio"
    }
}

# Direct CC0 sound effect URLs
SOUND_URLS = {
    # Footsteps
    "grass_01.wav": "https://freesound.org/data/previews/146/146723_2615119-lq.mp3",
    "stone_01.wav": "https://freesound.org/data/previews/416/416838_5121236-lq.mp3",
    "wood_01.wav": "https://freesound.org/data/previews/442/442910_907272-lq.mp3",
    
    # Combat
    "sword_swing.wav": "https://freesound.org/data/previews/65/65733_1013733-lq.mp3",
    "arrow_shoot.wav": "https://freesound.org/data/previews/442/442827_907272-lq.mp3",
    
    # Ambient
    "wind_light.wav": "https://freesound.org/data/previews/265/265549_4486188-lq.mp3",
    "rain_light.wav": "https://freesound.org/data/previews/270/270319_5123851-lq.mp3",
    "thunder_01.wav": "https://freesound.org/data/previews/416/416710_6489093-lq.mp3",
}

def download_file(url, dest, desc=""):
    """Download a file with error handling."""
    try:
        print(f"  Downloading {desc or dest.name}...")
        urllib.request.urlretrieve(url, dest)
        return True
    except Exception as e:
        print(f"    ✗ Failed: {e}")
        return False

def download_faithful_textures():
    """Download Faithful 32x32 texture pack."""
    print("\n[1/2] Downloading Faithful texture pack...")
    
    zip_path = TEMP_DIR / "faithful.zip"
    if not download_file(ASSET_SOURCES["faithful_textures"]["url"], zip_path, "Faithful pack"):
        return False
    
    print("  Extracting textures...")
    try:
        with zipfile.ZipFile(zip_path, 'r') as zf:
            # Extract only the texture files we need
            extract_dir = TEMP_DIR / "faithful"
            zf.extractall(extract_dir)
            
            # Find and copy block textures
            textures_copied = 0
            for root, dirs, files in os.walk(extract_dir):
                for file in files:
                    if file.endswith('.png'):
                        src = Path(root) / file
                        # Map to our block names
                        if 'stone' in file.lower():
                            dest = ASSETS_DIR / "textures" / "blocks" / "stone.png"
                        elif 'dirt' in file.lower():
                            dest = ASSETS_DIR / "textures" / "blocks" / "dirt.png"
                        elif 'grass' in file.lower() and 'top' in file.lower():
                            dest = ASSETS_DIR / "textures" / "blocks" / "grass_top.png"
                        elif 'grass' in file.lower() and 'side' in file.lower():
                            dest = ASSETS_DIR / "textures" / "blocks" / "grass_side.png"
                        elif 'oak' in file.lower() and 'log' in file.lower():
                            dest = ASSETS_DIR / "textures" / "blocks" / "wood.png"
                        elif 'oak' in file.lower() and 'plank' in file.lower():
                            dest = ASSETS_DIR / "textures" / "blocks" / "planks.png"
                        elif 'sand' in file.lower() and 'red' not in file.lower():
                            dest = ASSETS_DIR / "textures" / "blocks" / "sand.png"
                        elif 'water' in file.lower() and 'still' in file.lower():
                            dest = ASSETS_DIR / "textures" / "blocks" / "water.png"
                        elif 'lava' in file.lower() and 'still' in file.lower():
                            dest = ASSETS_DIR / "textures" / "blocks" / "lava.png"
                        elif 'glass' in file.lower() and 'stained' not in file.lower():
                            dest = ASSETS_DIR / "textures" / "blocks" / "glass.png"
                        elif 'leaves' in file.lower() and 'oak' in file.lower():
                            dest = ASSETS_DIR / "textures" / "blocks" / "leaves.png"
                        elif 'wool' in file.lower() and 'white' in file.lower():
                            dest = ASSETS_DIR / "textures" / "blocks" / "wool.png"
                        else:
                            continue
                        
                        shutil.copy2(src, dest)
                        textures_copied += 1
                        print(f"    ✓ Copied {file} -> {dest.name}")
            
            if textures_copied > 0:
                print(f"  ✓ Copied {textures_copied} textures")
                return True
            else:
                print("  ⚠️  No matching textures found")
                return False
                
    except Exception as e:
        print(f"  ✗ Failed to extract: {e}")
        return False

def download_sound_effects():
    """Download individual sound effects."""
    print("\n[2/2] Downloading sound effects...")
    
    downloaded = 0
    for filename, url in SOUND_URLS.items():
        # Determine destination directory
        if 'grass' in filename or 'stone' in filename or 'wood' in filename or 'sand' in filename or 'water' in filename:
            dest_dir = ASSETS_DIR / "sounds" / "player" / "footsteps"
        elif 'sword' in filename or 'arrow' in filename:
            dest_dir = ASSETS_DIR / "sounds" / "player" / "combat"
        elif 'wind' in filename or 'rain' in filename or 'thunder' in filename:
            dest_dir = ASSETS_DIR / "sounds" / "ambient"
        else:
            continue
        
        dest = dest_dir / filename
        if download_file(url, dest, filename):
            downloaded += 1
    
    if downloaded > 0:
        print(f"  ✓ Downloaded {downloaded} sound effects")
        return True
    else:
        print("  ⚠️  No sounds downloaded")
        return False

def create_improved_placeholders():
    """Create better quality placeholder assets using more sophisticated generation."""
    print("\nCreating improved placeholder assets...")
    
    # Use the existing placeholder generator but with better parameters
    import subprocess
    result = subprocess.run(
        [sys.executable, str(BASE_DIR / "scripts" / "create_placeholder_assets.py")],
        cwd=BASE_DIR
    )
    
    return result.returncode == 0

def main():
    print("=" * 70)
    print("Asset Replacement Script")
    print("Replacing placeholders with better quality assets")
    print("=" * 70)
    
    TEMP_DIR.mkdir(exist_ok=True)
    
    # Try downloading real assets
    textures_ok = download_faithful_textures()
    sounds_ok = download_sound_effects()
    
    if not textures_ok and not sounds_ok:
        print("\n⚠️  Could not download external assets")
        print("  Keeping existing placeholder assets")
    else:
        print("\n✓ Asset replacement complete!")
        if textures_ok:
            print("  ✓ Textures updated with Faithful pack")
        if sounds_ok:
            print("  ✓ Sound effects updated")
    
    # Cleanup
    if TEMP_DIR.exists():
        shutil.rmtree(TEMP_DIR)
    
    # Count final assets
    audio_count = sum(1 for _ in (ASSETS_DIR / "sounds").rglob("*.wav")) + \
                  sum(1 for _ in (ASSETS_DIR / "sounds").rglob("*.ogg")) + \
                  sum(1 for _ in (ASSETS_DIR / "sounds").rglob("*.mp3"))
    texture_count = sum(1 for _ in (ASSETS_DIR / "textures").rglob("*.png"))
    
    print("\n" + "=" * 70)
    print(f"Final asset count:")
    print(f"  Audio files: {audio_count}")
    print(f"  Texture files: {texture_count}")
    print("=" * 70)
    
    return 0

if __name__ == "__main__":
    sys.exit(main())
