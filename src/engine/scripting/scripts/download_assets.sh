#!/bin/bash
# Asset Download Script for Minecraft v2
# This script downloads all required free assets for the game

set -e

ASSETS_DIR="$(cd "$(dirname "$0")" && pwd)/assets"
TEMP_DIR="/tmp/minecraft_v2_assets"

echo "=== Minecraft v2 Asset Downloader ==="
echo "Assets will be downloaded to: $ASSETS_DIR"
echo ""

# Create temp directory
mkdir -p "$TEMP_DIR"
cd "$TEMP_DIR"

# Function to download and extract
# TODO: Medium - Add support for downloading assets from other sources, such as GitHub releases or other asset stores.
download_and_extract() {
    local url="$1"
    local filename="$2"
    local dest="$3"
    
    echo "Downloading $filename..."
    curl -L -o "$filename" "$url" || {
        echo "Failed to download $filename"
        return 1
    }
    
    if [[ "$filename" == *.zip ]]; then
        echo "Extracting $filename..."
        unzip -q "$filename" -d "$dest"
    elif [[ "$filename" == *.tar.gz ]]; then
        tar -xzf "$filename" -C "$dest"
    fi
}

# Download Kenney UI Pack (CC0)
# TODO: High - Automate the download and extraction of the Kenney UI packs.
# TODO: High - Add a check for the existence of the downloaded files to avoid re-downloading them every time the script is run.
echo ""
echo "=== Downloading Kenney UI Pack (CC0) ==="
mkdir -p "$ASSETS_DIR/textures/ui/kenney"
# Note: Direct download links for Kenney assets require visiting the website
# Manual download: https://kenney.nl/assets/ui-pack
echo "Please manually download Kenney UI Pack from:"
echo "  https://kenney.nl/assets/ui-pack"
echo "  Extract to: $ASSETS_DIR/textures/ui/kenney/"
echo ""

# Download Kenney Crosshair Pack (CC0)
echo "=== Downloading Kenney Crosshair Pack (CC0) ==="
echo "Please manually download Kenney Crosshair Pack from:"
echo "  https://kenney.nl/assets/crosshair-pack"
echo "  Extract to: $ASSETS_DIR/textures/ui/kenney/"
echo ""

# Download Kenney Particle Pack (CC0)
echo "=== Downloading Kenney Particle Pack (CC0) ==="
echo "Please manually download Kenney Particle Pack from:"
echo "  https://kenney.nl/assets/particle-pack"
echo "  Extract to: $ASSETS_DIR/textures/particles/kenney/"
echo ""

# Download sample sounds from Freesound (requires API key or manual download)
echo "=== Sound Effects ==="
echo "For sound effects, please visit Freesound.org and download:"
echo "  - Footstep sounds (grass, stone, wood)"
echo "  - Block break/place sounds"
echo "  - Water splash sounds"
echo "  - Ambient sounds (wind, rain, cave)"
echo ""
echo "Freesound.org: https://freesound.org/"
echo "Save to: $ASSETS_DIR/sounds/"
echo ""

# Download Kevin MacLeod music (CC BY 3.0)
echo "=== Downloading Kevin MacLeod Music (CC BY 3.0) ==="
echo "Please visit incompetech.com and download royalty-free music:"
echo "  https://incompetech.com/music/royalty-free/music.html"
echo "  Save to: $ASSETS_DIR/sounds/music/"
echo ""
echo "Remember to attribute: 'Music by Kevin MacLeod (incompetech.com)'"
echo "  Licensed under Creative Commons: By Attribution 3.0"
echo ""

# Create placeholder textures using Python
echo "=== Creating Placeholder Textures ==="
python3 << 'PYTHON_SCRIPT'
from PIL import Image, ImageDraw
import os

assets_dir = os.environ.get('ASSETS_DIR', 'assets')
hud_dir = os.path.join(assets_dir, 'textures', 'ui', 'hud')
os.makedirs(hud_dir, exist_ok=True)

# Create simple placeholder textures
def create_placeholder(filename, color, size=32):
    img = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    draw.rectangle([2, 2, size-3, size-3], fill=color, outline=(255, 255, 255, 255))
    img.save(os.path.join(hud_dir, filename))
    print(f"Created {filename}")

# HUD elements
create_placeholder('health_bar.png', (200, 50, 50, 255))
create_placeholder('hunger_bar.png', (200, 150, 50, 255))
create_placeholder('stamina_bar.png', (255, 200, 50, 255))
create_placeholder('hotbar.png', (100, 100, 100, 200), 364)
create_placeholder('hotbar_selection.png', (255, 255, 255, 100), 40)

# Crosshair
img = Image.new('RGBA', (32, 32), (0, 0, 0, 0))
draw = ImageDraw.Draw(img)
draw.line([(16, 8), (16, 24)], fill=(255, 255, 255, 255), width=2)
draw.line([(8, 16), (24, 16)], fill=(255, 255, 255, 255), width=2)
img.save(os.path.join(hud_dir, 'crosshair.png'))
print("Created crosshair.png")

print("Placeholder textures created successfully!")
PYTHON_SCRIPT

# Create block textures directory
echo ""
echo "=== Block Textures ==="
mkdir -p "$ASSETS_DIR/textures/blocks"
echo "For block textures, consider using:"
echo "  - Faithful 32x32: https://www.curseforge.com/minecraft/texture-packs/faithful-32x32"
echo "  - Or create your own 32x32 pixel art textures"
echo ""

# Cleanup
echo ""
echo "=== Cleanup ==="
cd -
rm -rf "$TEMP_DIR"

echo ""
echo "=== Asset Download Complete ==="
echo ""
echo "Next steps:"
echo "1. Manually download Kenney packs from the URLs above"
echo "2. Download sound effects from Freesound.org"
echo "3. Download music from incompetech.com"
echo "4. Organize files according to assets/ASSET_PLAN.md"
echo "5. Update assets/CREDITS.md with proper attribution"
echo ""
echo "Placeholder textures have been created in:"
echo "  $ASSETS_DIR/textures/ui/hud/"
echo ""
echo "For immediate testing, the placeholders will work!"
