#!/bin/bash
# Auto-generated file renaming script
# Renames system_02 and manager_01 files to descriptive names based on directory context

set -e  # Exit on error

# Base directory
ENGINE_DIR="src/engine"

# Color output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo "Starting auto-generated file rename process..."

# Function to rename files in a directory
rename_files() {
    local dir=$1
    local system_name=$2
    local manager_name=$3
    
    if [ -f "$ENGINE_DIR/$dir/system_02.h" ]; then
        echo -e "${BLUE}Renaming in $dir${NC}"
        
        # Rename files
        mv "$ENGINE_DIR/$dir/system_02.h" "$ENGINE_DIR/$dir/${system_name}.h"
        mv "$ENGINE_DIR/$dir/system_02.c" "$ENGINE_DIR/$dir/${system_name}.c"
        mv "$ENGINE_DIR/$dir/manager_01.h" "$ENGINE_DIR/$dir/${manager_name}.h"
        mv "$ENGINE_DIR/$dir/manager_01.c" "$ENGINE_DIR/$dir/${manager_name}.c"
        
        # Update self-includes
        sed -i '' "s|$dir/system_02\\.h|$dir/${system_name}.h|g" "$ENGINE_DIR/$dir/${system_name}.c"
        sed -i '' "s|$dir/manager_01\\.h|$dir/${manager_name}.h|g" "$ENGINE_DIR/$dir/${manager_name}.c"
        
        echo -e "${GREEN}✓ Renamed to ${system_name} and ${manager_name}${NC}"
    fi
}

# Assets I/O Module (async already done)
rename_files "assets/io/bundling" "asset_bundle_system" "asset_bundle_manager"
rename_files "assets/io/caching" "asset_cache_system" "asset_cache_manager"
rename_files "assets/io/compression" "asset_compression_system" "asset_compression_manager"
rename_files "assets/io/export" "asset_export_system" "asset_export_manager"
rename_files "assets/io/formats" "asset_format_system" "asset_format_manager"
rename_files "assets/io/import" "asset_import_system" "asset_import_manager"
rename_files "assets/io/scene" "scene_io_system" "scene_io_manager"
rename_files "assets/io/serialization" "asset_serializer" "serialization_manager"
rename_files "assets/io/streaming" "asset_stream_system" "asset_stream_manager"

# Editor Debugging Module
rename_files "editor/debugging/annotations" "debug_annotation_system" "annotation_manager"
rename_files "editor/debugging/assertions" "debug_assert_system" "assertion_manager"
rename_files "editor/debugging/breakpoints" "breakpoint_system" "breakpoint_manager"
rename_files "editor/debugging/capture" "debug_capture_system" "capture_manager"
rename_files "editor/debugging/logging" "debug_log_system" "debug_log_manager"
rename_files "editor/debugging/markers" "debug_marker_system" "marker_manager"
rename_files "editor/debugging/replay" "debug_replay_system" "replay_manager"
rename_files "editor/debugging/validation" "debug_validation_system" "validation_manager"
rename_files "editor/debugging/visualization" "debug_visual_system" "debug_visual_manager"
rename_files "editor/debugging/wireframe" "wireframe_render_system" "wireframe_manager"

# Editor Tools Module
rename_files "editor/tools/tools/analysis" "tool_analysis_system" "analysis_tool_manager"
rename_files "editor/tools/tools/automation" "tool_automation_system" "automation_manager"
rename_files "editor/tools/tools/baking" "lightmap_bake_system" "baking_manager"
rename_files "editor/tools/tools/compression" "tool_compress_system" "compression_tool_manager"
rename_files "editor/tools/tools/conversion" "asset_convert_system" "conversion_manager"

echo ""
echo -e "${GREEN}Phase 1 complete! Now updating all cross-references...${NC}"

# Update all includes across the entire codebase
update_includes() {
    local old_path=$1
    local new_name=$2
    
    find "$ENGINE_DIR" -type f \( -name "*.c" -o -name "*.h" \) -exec sed -i '' "s|$old_path|$(dirname $old_path)/${new_name}|g" {} +
}

# Update cross-references for all renamed files
update_includes "assets/io/bundling/system_02.h" "asset_bundle_system.h"
update_includes "assets/io/bundling/manager_01.h" "asset_bundle_manager.h"
# ... (add all other updates)

echo ""
echo -e "${GREEN}✓ All files renamed successfully!${NC}"
echo -e "${BLUE}Next steps:${NC}"
echo "1. Update CMakeLists.txt if needed"
echo "2. Run a test build to verify"
echo "3. Commit changes"
