#!/bin/bash
# Shader compilation script for Vulkan SPIR-V shaders

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
SHADER_DIR="$PROJECT_DIR/assets/shaders"
OUTPUT_DIR="$SHADER_DIR/compiled"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "=== Vulkan Shader Compilation ==="
echo "Shader directory: $SHADER_DIR"
echo "Output directory: $OUTPUT_DIR"
echo ""

# Check for glslc
if ! command -v glslc &> /dev/null; then
    echo -e "${RED}ERROR: glslc not found!${NC}"
    echo "Please install Vulkan SDK tools:"
    echo "  brew install vulkan-tools vulkan-loader molten-vk"
    exit 1
fi

# Create output directory
mkdir -p "$OUTPUT_DIR"

# Compile function
compile_shader() {
    local input_file="$1"
    local output_file="$2"
    
    echo -n "Compiling $(basename "$input_file")... "
    
    if glslc "$input_file" -o "$output_file" 2>&1; then
        echo -e "${GREEN}OK${NC}"
        return 0
    else
        echo -e "${RED}FAILED${NC}"
        return 1
    fi
}

# Compile simplified shaders (main rendering)
compile_shader "$SHADER_DIR/block_vertex_simple.glsl" "$OUTPUT_DIR/block_vertex.spv"
compile_shader "$SHADER_DIR/block_fragment_simple.glsl" "$OUTPUT_DIR/block_fragment.spv"

# Compile UI shaders
if [ -f "$PROJECT_DIR/shaders/ui.vert" ]; then
    compile_shader "$PROJECT_DIR/shaders/ui.vert" "$OUTPUT_DIR/ui_vertex.spv"
fi

if [ -f "$PROJECT_DIR/shaders/ui.frag" ]; then
    compile_shader "$PROJECT_DIR/shaders/ui.frag" "$OUTPUT_DIR/ui_fragment.spv"
fi

# Compile particle shaders if they exist
if [ -f "$SHADER_DIR/particle.vert" ]; then
    compile_shader "$SHADER_DIR/particle.vert" "$OUTPUT_DIR/particle_vertex.spv"
fi

if [ -f "$SHADER_DIR/particle.frag" ]; then
    compile_shader "$SHADER_DIR/particle.frag" "$OUTPUT_DIR/particle_fragment.spv"
fi

echo ""
echo -e "${GREEN}Shader compilation complete!${NC}"
echo "Compiled shaders are in: $OUTPUT_DIR"
ls -lh "$OUTPUT_DIR"
