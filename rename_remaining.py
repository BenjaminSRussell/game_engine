#!/usr/bin/env python3
"""Complete auto-generated file renaming for ALL subsystems."""
import os
from pathlib import Path

ENGINE_DIR = Path("src/engine")

# Complete rename mapping for ALL subsystems
RENAME_MAP = {
    # Physics Module
    "physics/broadphase": ("broadphase_system", "broadphase_manager"),
    "physics/cloth": ("cloth_physics_system", "cloth_physics_manager"),
    "physics/collision": ("collision_system", "collision_manager"),
    "physics/constraints": ("physics_constraint_system", "constraint_manager"),
    "physics/fluid": ("fluid_physics_system", "fluid_physics_manager"),
    "physics/narrowphase": ("narrowphase_system", "narrowphase_manager"),
    "physics/particles": ("particle_physics_system", "particle_physics_manager"),
    "physics/rigid": ("rigidbody_system", "rigidbody_manager"),
    "physics/soft": ("softbody_system", "softbody_manager"),
    "physics/solver": ("physics_solver_system", "physics_solver_manager"),
    
    # Editor Viewports
    "editor/viewports/camera": ("viewport_camera_system", "camera_manager"),
    "editor/viewports/cinematic": ("cinematic_camera_system", "cinematic_manager"),
    "editor/viewports/controllers": ("camera_controller_system", "controller_manager"),
    "editor/viewports/frustum": ("frustum_culling_system", "frustum_manager"),
    "editor/viewports/jitter": ("camera_jitter_system", "jitter_manager"),
    "editor/viewports/multiview": ("multiview_system", "multiview_manager"),
    "editor/viewports/projection": ("projection_system", "projection_manager"),
    "editor/viewports/split_screen": ("splitscreen_system", "splitscreen_manager"),
    "editor/viewports/temporal": ("temporal_aa_system", "temporal_manager"),
    "editor/viewports/vr": ("vr_viewport_system", "vr_viewport_manager"),
    
    # Editor Tools (additional)
    "editor/tools/tools/generation": ("content_gen_system", "generation_manager"),
    "editor/tools/tools/optimization": ("tool_optimize_system", "optimization_manager"),
    "editor/tools/tools/pipeline": ("asset_pipeline_system", "pipeline_manager"),
    "editor/tools/tools/shader_tools": ("shader_tool_system", "shader_tool_manager"),
    "editor/tools/tools/validation": ("asset_validation_system", "asset_validation_manager"),
}

def rename_directory(dir_rel, system_name, manager_name):
    """Rename files in a single directory."""
    dir_path = ENGINE_DIR / dir_rel
    
    if not dir_path.exists():
        return False, "Directory not found"
    
    files_to_rename = [
        ("system_02.h", f"{system_name}.h"),
        ("system_02.c", f"{system_name}.c"),
        ("manager_01.h", f"{manager_name}.h"),
        ("manager_01.c", f"{manager_name}.c"),
    ]
    
    # Check all files exist
    for old_name, _ in files_to_rename:
        if not (dir_path / old_name).exists():
            return False, f"Missing {old_name}"
    
    # Perform renames
    try:
        for old_name, new_name in files_to_rename:
            (dir_path / old_name).rename(dir_path / new_name)
        
        # Update self-includes in .c files
        for c_file in [f"{system_name}.c", f"{manager_name}.c"]:
            file_path = dir_path / c_file
            if file_path.exists():
                content = file_path.read_text()
                # Update include paths
                content = content.replace(f'"{dir_rel}/system_02.h"', f'"{dir_rel}/{system_name}.h"')
                content = content.replace(f'"{dir_rel}/manager_01.h"', f'"{dir_rel}/{manager_name}.h"')
                file_path.write_text(content)
        
        return True, "Success"
    except Exception as e:
        return False, str(e)

def main():
    print("=" * 70)
    print("Complete Auto-Generated File Renaming")
    print("=" * 70)
    
    results = {"success": [], "failed": []}
    
    for dir_rel, (sys_name, mgr_name) in sorted(RENAME_MAP.items()):
        success, msg = rename_directory(dir_rel, sys_name, mgr_name)
        status = "✓" if success else "✗"
        
        print(f"{status} {dir_rel:50s} -> {sys_name}, {mgr_name}")
        
        if success:
            results["success"].append(dir_rel)
        else:
            results["failed"].append((dir_rel, msg))
            print(f"   Error: {msg}")
    
    print("\n" + "=" * 70)
    print(f"✓ Success: {len(results['success'])} directories ({len(results['success']) * 2} file pairs)")
    print(f"✗ Failed:  {len(results['failed'])} directories")
    print("=" * 70)
    
    if results['failed']:
        print("\nFailed directories:")
        for dir_rel, msg in results['failed']:
            print(f"  - {dir_rel}: {msg}")

if __name__ == "__main__":
    main()
