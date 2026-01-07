#!/usr/bin/env python3
"""
Systematically rename all system_02 and manager_01 files to descriptive names.
"""
import os
import re
from pathlib import Path

# Base directory
ENGINE_DIR = Path("src/engine")

# Rename mapping based on directory context
RENAME_MAP = {
    # Assets I/O
    "assets/io/bundling": ("asset_bundle_system", "asset_bundle_manager"),
    "assets/io/caching": ("asset_cache_system", "asset_cache_manager"),
    "assets/io/compression": ("asset_compression_system", "asset_compression_manager"),
    "assets/io/export": ("asset_export_system", "asset_export_manager"),
    "assets/io/exporter": ("asset_exporter_system", "asset_exporter_manager"),
    "assets/io/formats": ("asset_format_system", "asset_format_manager"),
    "assets/io/import": ("asset_import_system", "asset_import_manager"),
    "assets/io/importer": ("asset_importer_system", "asset_importer_manager"),
    "assets/io/io": ("asset_io_system", "asset_io_manager"),
    "assets/io/scene": ("scene_io_system", "scene_io_manager"),
    "assets/io/serialization": ("asset_serializer", "serialization_manager"),
    "assets/io/streaming": ("asset_stream_system", "asset_stream_manager"),
    
    # Editor Debugging
    "editor/debugging/annotations": ("debug_annotation_system", "annotation_manager"),
    "editor/debugging/assertions": ("debug_assert_system", "assertion_manager"),
    "editor/debugging/breakpoints": ("breakpoint_system", "breakpoint_manager"),
    "editor/debugging/capture": ("debug_capture_system", "capture_manager"),
    "editor/debugging/logging": ("debug_log_system", "debug_log_manager"),
    "editor/debugging/markers": ("debug_marker_system", "marker_manager"),
    "editor/debugging/replay": ("debug_replay_system", "replay_manager"),
    "editor/debugging/validation": ("debug_validation_system", "validation_manager"),
    "editor/debugging/visualization": ("debug_visual_system", "debug_visual_manager"),
    "editor/debugging/wireframe": ("wireframe_render_system", "wireframe_manager"),
    
    # Editor Tools
    "editor/tools/tools/analysis": ("tool_analysis_system", "analysis_tool_manager"),
    "editor/tools/tools/automation": ("tool_automation_system", "automation_manager"),
    "editor/tools/tools/baking": ("lightmap_bake_system", "baking_manager"),
    "editor/tools/tools/compression": ("tool_compress_system", "compression_tool_manager"),
    "editor/tools/tools/conversion": ("asset_convert_system", "conversion_manager"),
}

def rename_files_in_dir(dir_path, system_name, manager_name):
    """Rename system_02 and manager_01 files in directory."""
    dir_path = ENGINE_DIR / dir_path
    
    if not dir_path.exists():
        print(f"⚠️  Directory not found: {dir_path}")
        return False
        
    files = {
        "system_h": dir_path / "system_02.h",
        "system_c": dir_path / "system_02.c",
        "manager_h": dir_path / "manager_01.h",
        "manager_c": dir_path / "manager_01.c",
    }
    
    # Check if files exist
    missing = [k for k, f in files.items() if not f.exists()]
    if missing:
        print(f"⚠️  Missing files in {dir_path}: {missing}")
        return False
    
    # Rename files
    try:
        files["system_h"].rename(dir_path / f"{system_name}.h")
        files["system_c"].rename(dir_path / f"{system_name}.c")
        files["manager_h"].rename(dir_path / f"{manager_name}.h")
        files["manager_c"].rename(dir_path / f"{manager_name}.c")
        
        # Update self-includes
        update_file_content(
            dir_path / f"{system_name}.c",
            f"assets/io/async/system_02.h" if "async" in str(dir_path) else f"{dir_path.relative_to(ENGINE_DIR)}/system_02.h",
            f"{dir_path.relative_to(ENGINE_DIR)}/{system_name}.h"
        )
        
        update_file_content(
            dir_path / f"{manager_name}.c",
            f"{dir_path.relative_to(ENGINE_DIR)}/manager_01.h",
            f"{dir_path.relative_to(ENGINE_DIR)}/{manager_name}.h"
        )
        
        print(f"✓ Renamed {dir_path.name}: {system_name}, {manager_name}")
        return True
    except Exception as e:
        print(f"✗ Error in {dir_path}: {e}")
        return False

def update_file_content(file_path, old_pattern, new_pattern):
    """Update file content by replacing pattern."""
    if not file_path.exists():
        return
    
    content = file_path.read_text()
    content = content.replace(old_pattern, new_pattern)
    file_path.write_text(content)

def main():
    print("=" * 60)
    print("Auto-Generated File Renaming Script")
    print("=" * 60)
    
    success_count = 0
    fail_count = 0
    
    for dir_rel, (system_name, manager_name) in RENAME_MAP.items():
        if rename_files_in_dir(dir_rel, system_name, manager_name):
            success_count += 1
        else:
            fail_count += 1
    
    print("\n" + "=" * 60)
    print(f"✓ Successfully renamed: {success_count} directories")
    print(f"✗ Failed: {fail_count} directories")
    print("=" * 60)
    
    print("\nNext steps:")
    print("1. Update all cross-references in other files")
    print("2. Update CMakeLists.txt if needed")
    print("3. Run test build")

if __name__ == "__main__":
    main()
