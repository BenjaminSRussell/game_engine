#!/bin/bash
# Asset Download and Setup Script
# This script downloads free CC0 assets and organizes them for the game

set -e  # Exit on error

ASSETS_DIR="$(cd "$(dirname "$0")" && pwd)"
TEMP_DIR="/tmp/minecraft_assets_$$"

echo "=== Minecraft v2 Asset Downloader ==="
echo "Downloading free CC0 assets..."
echo ""

# Create temp directory
mkdir -p "$TEMP_DIR"
cd "$TEMP_DIR"

# Function to download and extract
download_and_extract() {
    local url="$1"
    local filename="$2"
    local target_dir="$3"
    
    echo "Downloading $filename..."
    if curl -L -o "$filename" "$url"; then
        echo "Extracting to $target_dir..."
        mkdir -p "$ASSETS_DIR/$target_dir"
        unzip -q -o "$filename" -d "$ASSETS_DIR/$target_dir" 2>/dev/null || {
            echo "Warning: Could not extract $filename (may not be a zip file)"
            return 1
        }
        echo "✓ $filename installed"
        return 0
    else
        echo "✗ Failed to download $filename"
        return 1
    fi
}

# Download Kenney Particle Pack
echo ""
echo "--- Kenney Particle Pack ---"
echo "Note: This may require manual download from https://kenney.nl/assets/particle-pack"
echo "If automated download fails, please download manually and extract to:"
echo "  $ASSETS_DIR/textures/particles/kenney/"
echo ""

# Try direct download (may fail due to website structure)
if ! download_and_extract \
    "https://kenney.nl/media/pages/assets/particle-pack/fb76e09e85-1700582186/particlepack.zip" \
    "particle_pack.zip" \
    "textures/particles/kenney"; then
    echo ""
    echo "Automated download failed. Please download manually:"
    echo "1. Visit: https://kenney.nl/assets/particle-pack"
    echo "2. Click 'Download' button"
    echo "3. Extract to: $ASSETS_DIR/textures/particles/kenney/"
fi

# Download OpenGameArt Explosion
echo ""
echo "--- 16x16 Explosion Animation ---"
if ! download_and_extract \
    "https://opengameart.org/sites/default/files/16x16%20Explosion.zip" \
    "explosion.zip" \
    "textures/particles/explosion_oga"; then
    echo ""
    echo "Automated download failed. Please download manually:"
    echo "1. Visit: https://opengameart.org/content/16x16-explosion"
    echo "2. Download the zip file"
    echo "3. Extract to: $ASSETS_DIR/textures/particles/explosion_oga/"
fi

# Create simple colored NPC skin placeholders
echo ""
echo "--- Creating NPC Skin Placeholders ---"
mkdir -p "$ASSETS_DIR/textures/entities"

# Check if ImageMagick is available
if command -v convert &> /dev/null; then
    echo "Creating colored placeholder skins with ImageMagick..."
    
    # Villager (brown)
    convert -size 64x32 xc:"#8B4513" "$ASSETS_DIR/textures/entities/villager.png"
    
    # Zombie (green)
    convert -size 64x32 xc:"#00AA00" "$ASSETS_DIR/textures/entities/zombie.png"
    
    # Skeleton (white/bone)
    convert -size 64x32 xc:"#EEEEEE" "$ASSETS_DIR/textures/entities/skeleton.png"
    
    # Creeper (green with darker face)
    convert -size 64x32 xc:"#0DA70B" "$ASSETS_DIR/textures/entities/creeper.png"
    
    # Cow (brown/white)
    convert -size 64x32 xc:"#A0522D" "$ASSETS_DIR/textures/entities/cow.png"
    
    # Pig (pink)
    convert -size 64x32 xc:"#FFC0CB" "$ASSETS_DIR/textures/entities/pig.png"
    
    # Chicken (white)
    convert -size 64x32 xc:"#FFFFFF" "$ASSETS_DIR/textures/entities/chicken.png"
    
    echo "✓ Created 7 placeholder NPC skins"
else
    echo "ImageMagick not found. Skipping NPC skin generation."
    echo "Install with: brew install imagemagick"
fi

# Create simple particle placeholders if needed
echo ""
echo "--- Creating Additional Particle Placeholders ---"
if command -v convert &> /dev/null; then
    mkdir -p "$ASSETS_DIR/textures/particles/effects"
    
    # Fire particle (orange-red gradient)
    convert -size 16x16 radial-gradient:"#FF4500-#FF0000" \
        "$ASSETS_DIR/textures/particles/effects/fire.png"
    
    # Magic sparkle (cyan-white)
    convert -size 16x16 radial-gradient:"#00FFFF-#FFFFFF" \
        "$ASSETS_DIR/textures/particles/effects/magic.png"
    
    # Damage indicator (red)
    convert -size 16x16 xc:"#FF0000" \
        "$ASSETS_DIR/textures/particles/effects/damage.png"
    
    echo "✓ Created 3 additional particle effects"
fi

# Cleanup
cd "$ASSETS_DIR"
rm -rf "$TEMP_DIR"

echo ""
echo "=== Asset Setup Complete ==="
echo ""
echo "Summary:"
echo "- Explosion frames: $(ls -1 textures/particles/explosion/*.png 2>/dev/null | wc -l) files"
echo "- Particle effects: $(find textures/particles -name '*.png' 2>/dev/null | wc -l) total files"
echo "- NPC skins: $(ls -1 textures/entities/*.png 2>/dev/null | wc -l) files"
echo ""
echo "Next steps:"
echo "1. Review ASSET_SOURCES.md for higher quality asset sources"
echo "2. Replace placeholders with proper assets as needed"
echo "3. Add attribution to CREDITS.md for any CC-BY assets"
echo ""
echo "The game is ready to run with current placeholder assets!"
