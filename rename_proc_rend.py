#!/usr/bin/env python3
"""Rename processor_04 and renderer_03 files to descriptive names."""
from pathlib import Path

ENGINE_DIR = Path("src/engine")

# Rename mapping: processor_04 → processor name, renderer_03 → renderer name
RENAME_MAP = {
    # Assets I/O
    "assets/io/async": ("async_io_processor", "async_io_renderer"),
    "assets/io/bundling": ("bundle_processor", "bundle_renderer"),
    "assets/io/caching": ("cache_processor", "cache_renderer"),
    "assets/io/compression": ("compression_processor", "compression_renderer"),
    "assets/io/export": ("asset_export_processor", "asset_export_renderer"),
    "assets/io/formats": ("format_processor", "format_renderer"),
    "assets/io/import": ("asset_import_processor", "asset_import_renderer"),
    "assets/io/scene": ("scene_processor", "scene_renderer"),
    "assets/io/serialization": ("serialization_processor", "serialization_renderer"),
    "assets/io/streaming": ("stream_processor", "stream_renderer"),
    
    # Editor Debugging
    "editor/debugging/annotations": ("annotation_processor", "annotation_renderer"),
    "editor/debugging/assertions": ("assert_processor", "assert_renderer"),
    "editor/debugging/breakpoints": ("breakpoint_processor", "breakpoint_renderer"),
    "editor/debugging/capture": ("capture_processor", "capture_renderer"),
    "editor/debugging/logging": ("log_processor", "log_renderer"),
    "editor/debugging/markers": ("marker_processor", "marker_renderer"),
    "editor/debugging/replay": ("replay_processor", "replay_renderer"),
    "editor/debugging/validation": ("validation_processor", "validation_renderer"),
    "editor/debugging/visualization": ("debug_viz_processor", "debug_viz_renderer"),
    "editor/debugging/wireframe": ("wireframe_processor", "wireframe_renderer"),
    
    # Editor Tools
    "editor/tools/tools/analysis": ("analysis_processor", "analysis_renderer"),
    "editor/tools/tools/automation": ("automation_processor", "automation_renderer"),
    "editor/tools/tools/baking": ("bake_processor", "bake_renderer"),
    "editor/tools/tools/compression": ("tool_compress_processor", "tool_compress_renderer"),
    "editor/tools/tools/conversion": ("conversion_processor", "conversion_renderer"),
    "editor/tools/tools/generation": ("generation_processor", "generation_renderer"),
    "editor/tools/tools/optimization": ("optimize_processor", "optimize_renderer"),
    "editor/tools/tools/pipeline": ("pipeline_processor", "pipeline_renderer"),
    "editor/tools/tools/shader_tools": ("shader_processor", "shader_renderer"),
    "editor/tools/tools/validation": ("asset_val_processor", "asset_val_renderer"),
    
    # Editor Viewports
    "editor/viewports/camera": ("camera_processor", "camera_renderer"),
    "editor/viewports/cinematic": ("cinematic_processor", "cinematic_renderer"),
    "editor/viewports/controllers": ("controller_processor", "controller_renderer"),
    "editor/viewports/frustum": ("frustum_processor", "frustum_renderer"),
    "editor/viewports/jitter": ("jitter_processor", "jitter_renderer"),
    "editor/viewports/multiview": ("multiview_processor", "multiview_renderer"),
    "editor/viewports/projection": ("projection_processor", "projection_renderer"),
    "editor/viewports/split_screen": ("splitscreen_processor", "splitscreen_renderer"),
    "editor/viewports/temporal": ("temporal_processor", "temporal_renderer"),
    "editor/viewports/vr": ("vr_processor", "vr_renderer"),
    
    # Physics
    "physics/broadphase": ("broadphase_processor", "broadphase_renderer"),
    "physics/cloth": ("cloth_processor", "cloth_renderer"),
    "physics/collision": ("collision_processor", "collision_renderer"),
    "physics/constraints": ("constraint_processor", "constraint_renderer"),
    "physics/fluid": ("fluid_processor", "fluid_renderer"),
    "physics/narrowphase": ("narrowphase_processor", "narrowphase_renderer"),
    "physics/particles": ("particle_processor", "particle_renderer"),
    "physics/rigid": ("rigidbody_processor", "rigidbody_renderer"),
    "physics/soft": ("softbody_processor", "softbody_renderer"),
    "physics/solver": ("solver_processor", "solver_renderer"),
}

def rename_directory(dir_rel, proc_name, rend_name):
    """Rename processor_04 and renderer_03 files in directory."""
    dir_path = ENGINE_DIR / dir_rel
    
    if not dir_path.exists():
        return False, "Directory not found"
    
    files = [
        ("processor_04.h", f"{proc_name}.h"),
        ("processor_04.c", f"{proc_name}.c"),
        ("renderer_03.h", f"{rend_name}.h"),
        ("renderer_03.c", f"{rend_name}.c"),
    ]
    
    # Check files exist
    for old_name, _ in files:
        if not (dir_path / old_name).exists():
            return False, f"Missing {old_name}"
    
    # Rename files
    try:
        for old_name, new_name in files:
            (dir_path / old_name).rename(dir_path / new_name)
        
        # Update self-includes
        for c_file in [f"{proc_name}.c", f"{rend_name}.c"]:
            file_path = dir_path / c_file
            if file_path.exists():
                content = file_path.read_text()
                content = content.replace(f'"{dir_rel}/processor_04.h"', f'"{dir_rel}/{proc_name}.h"')
                content = content.replace(f'"{dir_rel}/renderer_03.h"', f'"{dir_rel}/{rend_name}.h"')
                file_path.write_text(content)
        
        return True, "Success"
    except Exception as e:
        return False, str(e)

def main():
    print("=" * 70)
    print("Processor & Renderer File Renaming")
    print("=" * 70)
    
    success = []
    failed = []
    
    for dir_rel, (proc, rend) in sorted(RENAME_MAP.items()):
        ok, msg = rename_directory(dir_rel, proc, rend)
        status = "✓" if ok else "✗"
        
        print(f"{status} {dir_rel:50s} -> {proc}, {rend}")
        
        if ok:
            success.append(dir_rel)
        else:
            failed.append((dir_rel, msg))
            print(f"   Error: {msg}")
    
    print("\n" + "=" * 70)
    print(f"✓ Success: {len(success)} directories ({len(success) * 2} file pairs)")
    print(f"✗ Failed:  {len(failed)} directories")
    print("=" * 70)

if __name__ == "__main__":
    main()
