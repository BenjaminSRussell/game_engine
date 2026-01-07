#!/bin/bash
# Asset Dependency Downloader
# Downloads required header libraries and sets up asset directories

set -e  # Exit on error

PROJECT_ROOT="/Users/benjaminrussell/Desktop/Minecraft v2"
VENDOR_DIR="$PROJECT_ROOT/include/vendor"

echo "🚀 Minecraft v2 - Asset Dependency Setup"
echo "========================================"
echo ""

# Create directories
echo "📁 Creating directory structure..."
mkdir -p "$VENDOR_DIR"
mkdir -p "$PROJECT_ROOT/assets/models/characters"
mkdir -p "$PROJECT_ROOT/assets/models/items"
mkdir -p "$PROJECT_ROOT/assets/models/environment"
mkdir -p "$PROJECT_ROOT/assets/textures/atlas"
mkdir -p "$PROJECT_ROOT/assets/textures/blocks"
mkdir -p "$PROJECT_ROOT/assets/textures/items"
mkdir -p "$PROJECT_ROOT/assets/textures/ui"
mkdir -p "$PROJECT_ROOT/assets/textures/particles"
mkdir -p "$PROJECT_ROOT/assets/sounds/player/footsteps"
mkdir -p "$PROJECT_ROOT/assets/sounds/player/combat"
mkdir -p "$PROJECT_ROOT/assets/sounds/player/interact"
mkdir -p "$PROJECT_ROOT/assets/sounds/mobs"
mkdir -p "$PROJECT_ROOT/assets/sounds/blocks"
mkdir -p "$PROJECT_ROOT/assets/sounds/ambient"
mkdir -p "$PROJECT_ROOT/assets/music/overworld"
mkdir -p "$PROJECT_ROOT/assets/music/combat"
mkdir -p "$PROJECT_ROOT/assets/music/menu"
mkdir -p "$PROJECT_ROOT/test"
echo "✅ Directories created"
echo ""

# Download cgltf
echo "📥 Downloading cgltf (GLTF 2.0 loader)..."
cd "$VENDOR_DIR"
if [ -f "cgltf.h" ]; then
    echo "   ⏭️  cgltf.h already exists, skipping"
else
    curl -L -o cgltf.h https://raw.githubusercontent.com/jkuhlmann/cgltf/master/cgltf.h
    if [ -f "cgltf.h" ]; then
        SIZE=$(wc -c < cgltf.h | tr -d ' ')
        echo "   ✅ Downloaded cgltf.h ($SIZE bytes)"
    else
        echo "   ❌ Failed to download cgltf.h"
        exit 1
    fi
fi
echo ""

# Download stb_image
echo "📥 Downloading stb_image (image loader)..."
cd "$VENDOR_DIR"
if [ -f "stb_image.h" ]; then
    echo "   ⏭️  stb_image.h already exists, skipping"
else
    curl -L -o stb_image.h https://raw.githubusercontent.com/nothings/stb/master/stb_image.h
    if [ -f "stb_image.h" ]; then
        SIZE=$(wc -c < stb_image.h | tr -d ' ')
        echo "   ✅ Downloaded stb_image.h ($SIZE bytes)"
    else
        echo "   ❌ Failed to download stb_image.h"
        exit 1
    fi
fi
echo ""

# Create test GLTF loader
echo "📝 Creating test GLTF loader..."
cd "$PROJECT_ROOT"
cat > test/test_gltf_load.c << 'EOF'
#define CGLTF_IMPLEMENTATION
#include "../include/vendor/cgltf.h"
#include <stdio.h>

int main(int argc, char **argv) {
    const char *path = (argc > 1) ? argv[1] : "assets/models/characters/spirit.gltf";

    cgltf_options options = {0};
    cgltf_data *data = NULL;

    printf("Loading GLTF: %s\n", path);

    cgltf_result result = cgltf_parse_file(&options, path, &data);
    if (result != cgltf_result_success) {
        printf("❌ Failed to parse GLTF: error %d\n", result);
        return 1;
    }

    result = cgltf_load_buffers(&options, data, path);
    if (result != cgltf_result_success) {
        printf("❌ Failed to load buffers: error %d\n", result);
        cgltf_free(data);
        return 1;
    }

    printf("\n✅ Loaded GLTF successfully!\n\n");
    printf("Model Information:\n");
    printf("  Meshes:     %zu\n", data->meshes_count);
    printf("  Nodes:      %zu\n", data->nodes_count);
    printf("  Materials:  %zu\n", data->materials_count);
    printf("  Textures:   %zu\n", data->textures_count);
    printf("  Animations: %zu\n", data->animations_count);
    printf("  Skins:      %zu\n", data->skins_count);

    if (data->meshes_count > 0) {
        printf("\nFirst Mesh:\n");
        cgltf_mesh *mesh = &data->meshes[0];
        printf("  Name:       %s\n", mesh->name ? mesh->name : "(unnamed)");
        printf("  Primitives: %zu\n", mesh->primitives_count);

        if (mesh->primitives_count > 0) {
            cgltf_primitive *prim = &mesh->primitives[0];
            printf("  Attributes: %zu\n", prim->attributes_count);

            for (size_t i = 0; i < prim->attributes_count; i++) {
                cgltf_attribute *attr = &prim->attributes[i];
                const char *type_name = "UNKNOWN";

                switch (attr->type) {
                    case cgltf_attribute_type_position: type_name = "POSITION"; break;
                    case cgltf_attribute_type_normal: type_name = "NORMAL"; break;
                    case cgltf_attribute_type_texcoord: type_name = "TEXCOORD"; break;
                    case cgltf_attribute_type_joints: type_name = "JOINTS"; break;
                    case cgltf_attribute_type_weights: type_name = "WEIGHTS"; break;
                    default: break;
                }

                printf("    - %-12s: %zu elements\n", type_name, attr->data->count);
            }

            if (prim->indices) {
                printf("  Indices:    %zu\n", prim->indices->count);
            }
        }
    }

    if (data->animations_count > 0) {
        printf("\nAnimations:\n");
        for (size_t i = 0; i < data->animations_count; i++) {
            cgltf_animation *anim = &data->animations[i];
            printf("  [%zu] %s - %zu channels\n",
                   i,
                   anim->name ? anim->name : "(unnamed)",
                   anim->channels_count);
        }
    }

    cgltf_free(data);
    return 0;
}
EOF
echo "✅ Created test/test_gltf_load.c"
echo ""

# Create Makefile for test
echo "📝 Creating test Makefile..."
cat > test/Makefile << 'EOF'
CC = gcc
CFLAGS = -I../include -Wall -Wextra -O2
LDFLAGS = -lm

.PHONY: all clean

all: test_gltf

test_gltf: test_gltf_load.c
	$(CC) $(CFLAGS) -o test_gltf test_gltf_load.c $(LDFLAGS)

clean:
	rm -f test_gltf

test: test_gltf
	@echo "Testing with sample GLTF..."
	./test_gltf ../assets/models/characters/spirit.gltf || echo "No model found yet - download from Mixamo!"
EOF
echo "✅ Created test/Makefile"
echo ""

# Create README for assets directory
echo "📝 Creating assets README..."
cat > "$PROJECT_ROOT/assets/README.md" << 'EOF'
# Assets Directory

This directory contains all game assets including 3D models, textures, audio, and music.

## Directory Structure

```
assets/
├── models/          # 3D models (GLTF format)
│   ├── characters/  # Player and NPC models with animations
│   ├── items/       # Tools, weapons, items
│   └── environment/ # Trees, structures, decorations
├── textures/        # All textures and sprites
│   ├── atlas/       # Packed texture atlases
│   ├── blocks/      # Individual block textures (before packing)
│   ├── items/       # Item icons
│   ├── ui/          # UI elements
│   └── particles/   # Particle effect sprites
├── sounds/          # Sound effects (WAV/OGG)
│   ├── player/      # Player actions (footsteps, combat)
│   ├── mobs/        # NPC sounds
│   ├── blocks/      # Block interactions
│   └── ambient/     # Environment sounds (wind, rain)
└── music/           # Background music (OGG)
    ├── overworld/   # Exploration music
    ├── combat/      # Battle music
    └── menu/        # Menu theme
```

## Getting Assets

See documentation:
- `docs/QUICK_START_ASSETS.md` - Download first assets (1-2 hours)
- `docs/ASSET_ACQUISITION_ROADMAP.md` - Complete asset plan
- `docs/ASSET_LOADER_IMPLEMENTATION.md` - Technical integration

## Quick Start

1. Download Mixamo character: https://www.mixamo.com
   - Character: "Y Bot"
   - Format: GLTF 2.0
   - Save as: `models/characters/spirit.gltf`

2. Download animations (same character):
   - Idle, Walking, Running, Jumping, Attack
   - Save as: `models/characters/spirit_*.gltf`

3. Download block textures:
   - Faithful 32x32: https://faithful.team
   - Or Kenney Voxel Pack: https://kenney.nl/assets/voxel-pack
   - Place in: `textures/blocks/`

4. Download sounds:
   - Freesound.org: https://freesound.org
   - Search: "footsteps", "sword", "block break"
   - Place in: `sounds/player/` and `sounds/blocks/`

## Attribution

All assets must include proper attribution. Add to `../CREDITS.md`:

```markdown
## 3D Models
- Spirit Character: Mixamo (free license)

## Textures
- Block textures: Faithful Team (CC0)

## Audio
- Footsteps: Freesound user "InspectorJ" (CC-BY 3.0)
```

## License Compatibility

- ✅ CC0 (Public Domain) - No attribution needed, best choice
- ✅ CC-BY - Attribution required, commercial use OK
- ⚠️ CC-BY-SA - Attribution + share-alike, use with caution
- ❌ CC-BY-NC - Non-commercial only, DO NOT USE

For more details, see `docs/ASSET_ACQUISITION_ROADMAP.md`
EOF
echo "✅ Created assets/README.md"
echo ""

# Summary
echo "✨ Setup Complete!"
echo "================="
echo ""
echo "Downloaded:"
echo "  ✅ cgltf.h - GLTF 2.0 model loader"
echo "  ✅ stb_image.h - Image/texture loader"
echo ""
echo "Created:"
echo "  ✅ Asset directory structure"
echo "  ✅ Test GLTF loader (test/test_gltf_load.c)"
echo "  ✅ Test Makefile (test/Makefile)"
echo "  ✅ Assets README (assets/README.md)"
echo ""
echo "Next Steps:"
echo "  1. Download Mixamo character: https://www.mixamo.com"
echo "     → Search 'Y Bot', download as GLTF 2.0"
echo "     → Save to: assets/models/characters/spirit.gltf"
echo ""
echo "  2. Test GLTF loading:"
echo "     cd test && make && ./test_gltf"
echo ""
echo "  3. Follow guide: docs/QUICK_START_ASSETS.md"
echo ""
echo "📚 Documentation:"
echo "  - docs/QUICK_START_ASSETS.md (start here!)"
echo "  - docs/ASSET_LOADER_IMPLEMENTATION.md"
echo "  - docs/ASSET_ACQUISITION_ROADMAP.md"
echo "  - docs/ASSET_PLAN_SUMMARY.md"
echo ""
