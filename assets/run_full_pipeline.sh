#!/bin/bash
# Full Pipeline Script for Modpack Asset Acquisition
# Runs all steps in sequence: download -> extract -> organize -> optimize

set -e  # Exit on error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "============================================================"
echo "Modpack Asset Acquisition - Full Pipeline"
echo "============================================================"
echo ""

# Check for Python
if ! command -v python3 &> /dev/null; then
    echo "Error: python3 is required but not installed."
    exit 1
fi

# Install dependencies if needed
if [ ! -d "venv" ]; then
    echo "Creating virtual environment..."
    python3 -m venv venv
    source venv/bin/activate
    echo "Installing dependencies..."
    pip install -q -r requirements.txt
else
    source venv/bin/activate
fi

# Step 1: Download modpacks (optional - can skip if already downloaded)
echo ""
echo "============================================================"
echo "Step 1: Download Modpacks"
echo "============================================================"
read -p "Download modpacks? (y/N): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    python3 download_modpacks.py
else
    echo "Skipping download step. Ensure modpacks are in downloads/ directory."
fi

# Step 2: Extract assets from JARs
echo ""
echo "============================================================"
echo "Step 2: Extract Assets from JARs"
echo "============================================================"
python3 extract_modpack_assets.py

# Step 3: Organize extracted assets
echo ""
echo "============================================================"
echo "Step 3: Organize Extracted Assets"
echo "============================================================"
python3 organize_extracted_assets.py

# Step 4: Optimize textures
echo ""
echo "============================================================"
echo "Step 4: Optimize Textures"
echo "============================================================"
read -p "Optimize textures? Requires Pillow (y/N): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    python3 optimize_textures.py
else
    echo "Skipping texture optimization."
fi

# Step 5: Convert models (optional - not yet fully implemented)
echo ""
echo "============================================================"
echo "Step 5: Convert Models (Optional)"
echo "============================================================"
read -p "Attempt model conversion? (Note: Not yet fully implemented) (y/N): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    python3 convert_models.py
else
    echo "Skipping model conversion."
fi

echo ""
echo "============================================================"
echo "Pipeline Complete!"
echo "============================================================"
echo ""
echo "Next steps:"
echo "1. Review extracted assets in: extracted/"
echo "2. Check licensing: licensing/README.md"
echo "3. Review optimization results in: optimized/"
echo "4. Integrate selected assets into main assets/ directory"
echo ""
echo "See MODPACK_ACQUISITION_GUIDE.md for detailed usage."

