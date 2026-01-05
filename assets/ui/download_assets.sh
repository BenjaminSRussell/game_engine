#!/bin/bash

# UI Asset Download Script
# Downloads free UI assets from various sources for the Swift frontend

set -e

ASSETS_DIR="/Users/benjaminrussell/Desktop/Minecraft v2/assets/ui"
TEMP_DIR="/tmp/minecraft_ui_assets"

echo "🎨 Minecraft v2 - UI Asset Downloader"
echo "======================================"
echo ""

# Create temp directory
mkdir -p "$TEMP_DIR"
cd "$TEMP_DIR"

echo "📦 Downloading UI asset packs..."
echo ""

# Function to download and extract
download_and_extract() {
    local url=$1
    local filename=$2
    local dest_dir=$3
    
    echo "⬇️  Downloading $filename..."
    if command -v curl &> /dev/null; then
        curl -L -o "$filename" "$url" || echo "⚠️  Failed to download $filename"
    elif command -v wget &> /dev/null; then
        wget -O "$filename" "$url" || echo "⚠️  Failed to download $filename"
    else
        echo "❌ Error: Neither curl nor wget is available"
        return 1
    fi
    
    # Extract if it's a zip file
    if [[ $filename == *.zip ]]; then
        echo "📂 Extracting $filename..."
        unzip -q "$filename" -d "$dest_dir" || echo "⚠️  Failed to extract $filename"
    fi
}

# Note: These are placeholder URLs - you'll need to manually download from the sources
# as many require clicking through license agreements

echo "📝 Asset Sources:"
echo ""
echo "Please manually download assets from these sources:"
echo ""
echo "1. Game-icons.net (4170+ free icons)"
echo "   URL: https://game-icons.net/"
echo "   Download: Click 'Download all icons' button"
echo "   License: CC BY 3.0"
echo ""
echo "2. OpenGameArt.org - UI Pack"
echo "   URL: https://opengameart.org/content/ui-pack"
echo "   License: CC0 (Public Domain)"
echo ""
echo "3. itch.io - Free UI Packs"
echo "   Search: https://itch.io/game-assets/free/tag-user-interface"
echo "   Recommended packs:"
echo "   - Pixel Art GUI Elements"
echo "   - Fantasy RPG Icons"
echo "   - Sci-Fi UI Collection"
echo ""
echo "4. Vecteezy - Game UI Vectors"
echo "   URL: https://www.vecteezy.com/free-vector/game-ui"
echo "   Filter: Free vectors only"
echo ""
echo "5. CraftPix.net - Free 2D Assets"
echo "   URL: https://craftpix.net/freebies/"
echo "   Category: GUI"
echo ""

# Create asset organization structure
echo "📁 Creating asset directory structure..."
mkdir -p "$ASSETS_DIR/icons/items"
mkdir -p "$ASSETS_DIR/icons/status"
mkdir -p "$ASSETS_DIR/icons/ui"
mkdir -p "$ASSETS_DIR/buttons/primary"
mkdir -p "$ASSETS_DIR/buttons/secondary"
mkdir -p "$ASSETS_DIR/backgrounds/menu"
mkdir -p "$ASSETS_DIR/backgrounds/panel"
mkdir -p "$ASSETS_DIR/borders/panel"
mkdir -p "$ASSETS_DIR/effects/particles"

echo "✅ Directory structure created"
echo ""

# Generate placeholder icons using Python
echo "🎨 Generating placeholder icons..."

python3 << 'PYTHON_SCRIPT'
from PIL import Image, ImageDraw, ImageFont
import os

assets_dir = "/Users/benjaminrussell/Desktop/Minecraft v2/assets/ui"

# Icon definitions
icons = {
    "status": [
        ("health", (220, 50, 50)),
        ("hunger", (220, 140, 50)),
        ("armor", (100, 150, 220)),
        ("experience", (100, 220, 100)),
        ("oxygen", (80, 200, 220)),
    ],
    "items": [
        ("sword", (200, 200, 200)),
        ("pickaxe", (150, 150, 150)),
        ("axe", (140, 90, 60)),
        ("shovel", (120, 120, 120)),
        ("bow", (160, 100, 60)),
        ("arrow", (180, 180, 180)),
    ],
    "ui": [
        ("settings", (150, 150, 150)),
        ("inventory", (180, 140, 100)),
        ("map", (100, 180, 100)),
        ("quest", (220, 180, 80)),
        ("close", (220, 80, 80)),
    ]
}

def create_icon(name, color, size=64):
    """Create a simple placeholder icon"""
    img = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    
    # Draw circle background
    padding = 4
    draw.ellipse([padding, padding, size-padding, size-padding], fill=color)
    
    # Draw border
    draw.ellipse([padding, padding, size-padding, size-padding], outline=(255, 255, 255, 200), width=2)
    
    # Draw letter
    try:
        font = ImageFont.truetype("/System/Library/Fonts/Helvetica.ttc", size//2)
    except:
        font = ImageFont.load_default()
    
    letter = name[0].upper()
    bbox = draw.textbbox((0, 0), letter, font=font)
    text_width = bbox[2] - bbox[0]
    text_height = bbox[3] - bbox[1]
    text_x = (size - text_width) // 2
    text_y = (size - text_height) // 2 - 4
    
    draw.text((text_x, text_y), letter, fill=(255, 255, 255, 255), font=font)
    
    return img

# Generate icons
for category, icon_list in icons.items():
    category_dir = os.path.join(assets_dir, "icons", category)
    os.makedirs(category_dir, exist_ok=True)
    
    for name, color in icon_list:
        # Generate @2x and @3x versions
        for scale in [2, 3]:
            size = 32 * scale
            icon = create_icon(name, color, size)
            filename = f"icon_{category}_{name}@{scale}x.png"
            filepath = os.path.join(category_dir, filename)
            icon.save(filepath)
            print(f"✓ Created {filename}")

print("\n✅ Placeholder icons generated")
PYTHON_SCRIPT

echo ""
echo "✅ Asset setup complete!"
echo ""
echo "📋 Next steps:"
echo "1. Download assets from the sources listed above"
echo "2. Extract and organize them into the appropriate directories"
echo "3. Replace placeholder icons with downloaded assets"
echo "4. Run texture optimization: python3 assets/optimize_textures.py"
echo ""
echo "Asset directory: $ASSETS_DIR"
echo ""

# Cleanup
cd -
rm -rf "$TEMP_DIR"
