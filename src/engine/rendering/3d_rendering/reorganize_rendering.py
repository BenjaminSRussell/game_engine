#!/usr/bin/env python3
"""
3D Rendering Directory Reorganization Script

This script reorganizes the 3d_rendering directory from 73 flat folders
into a clean hierarchy of ~20 top-level categories.

Features:
- Git-aware file moving (preserves history via git mv)
- Automatic #include path updates in .c/.h files
- Dry-run mode for safe preview
- Detailed mapping and logging

Usage:
    python reorganize_rendering.py --dry-run          # Preview changes
    python reorganize_rendering.py --execute           # Execute reorganization
    python reorganize_rendering.py --generate-mapping  # Generate mapping file only
"""

import os
import sys
import json
import argparse
import subprocess
import re
from pathlib import Path
from typing import Dict, List, Tuple
from collections import defaultdict

# Base directory (auto-detect from script location)
SCRIPT_DIR = Path(__file__).parent.resolve()
BASE_DIR = SCRIPT_DIR

# Directory reorganization mapping
# Format: "old_path": "new_path"
DIRECTORY_MAPPING = {
    # Backend
    "platform/metal": "backend/metal",
    
    # Geometry additions
    "nanite": "geometry/nanite",
    "procedural_mesh": "geometry/procedural_mesh",
    
    # Lighting additions
    "lumen": "lighting/lumen",
    "virtual_shadow_maps": "lighting/vsm",
    "shadows_advanced": "lighting/shadows_advanced",
    
    # Effects consolidation
    "fire_rendering": "effects/fire",
    "smoke_rendering": "effects/smoke",
    "water": "effects/water",
    "destruction": "effects/destruction",
    "explosion_effects": "effects/explosions",
    "footprints": "effects/footprints",
    "puddles": "effects/puddles",
    "decals": "effects/decals",
    
    # Environment
    "landscape": "environment/landscape/terrain",
    "landscape_grass": "environment/landscape/grass",
    "atmosphere": "environment/atmosphere/core",
    "sky_atmosphere": "environment/atmosphere/sky",
    "volumetric_clouds": "environment/clouds",
    "vegetation_rendering_advanced": "environment/vegetation",
    
    # Character
    "animation": "character/animation",
    "skin_rendering": "character/skin",
    "eye_rendering": "character/eyes",
    "hair_rendering": "character/hair/rendering",
    "hair_system": "character/hair/system",
    "cloth_system": "character/cloth",
    "skeletal_mesh_rendering": "character/skeletal_mesh",
    
    # Materials - specific material types
    "concrete_rendering": "materials/types/concrete",
    "fabric_rendering": "materials/types/fabric",
    "glass_rendering": "materials/types/glass",
    "liquid_rendering": "materials/types/liquid",
    "metal_rendering": "materials/types/metal",
    "stone_rendering": "materials/types/stone",
    "wood_rendering": "materials/types/wood",
    
    # Materials - weathering
    "age_weathering": "materials/weathering/age",
    "rust_corrosion": "materials/weathering/rust",
    "dirt_grunge": "materials/weathering/dirt",
    "wear_tear": "materials/weathering/wear",
    "wetness_system": "materials/wetness",
    
    # Assets
    "asset_system": "assets/system",
    "io": "assets/io",
    "texture": "assets/textures",
    "resource_management": "assets/resources",
    
    # Core additions
    "framebuffer": "core/framebuffer",
    "physics": "core/physics",
    "math": "core/math",
    "scene_management": "core/scene",
    
    # Rendering additions
    "gpu_scene": "rendering/gpu_scene",
    "occlusion": "rendering/occlusion",
    "culling": "rendering/culling",
    "raytracing": "rendering/raytracing",
    "bindless_rendering": "rendering/bindless",
    "vrs": "rendering/vrs",
    "static_mesh_rendering": "rendering/static_mesh",
    
    # Postprocessing (rename)
    "postprocessing": "postprocess",
    "temporal_upscaling": "postprocess/temporal_upscaling",
    
    # Editor additions
    "viewports": "editor/viewports",
    "ui_rendering": "editor/ui",
    "debugging": "editor/debugging",
    "tools": "editor/tools",
    
    # Profiling additions
    "statistics": "profiling/statistics",
    
    # Misc
    "scripting": "tools/scripting",
    "world_partition": "scene/world_partition",
    "agents": "scene/agents",
    
    # Directories that stay in place (no mapping needed, but listed for completeness)
    # "core": "core",
    # "geometry": "geometry",
    # "lighting": "lighting",
    # "materials": "materials",
    # "shading": "shading",
    # "rendering": "rendering",
    # "effects": "effects",
    # "editor": "editor",
    # "profiling": "profiling",
}


class ReorganizationScript:
    def __init__(self, base_dir: Path, dry_run: bool = True):
        self.base_dir = base_dir
        self.dry_run = dry_run
        self.operations_log = []
        self.include_updates = []
        self.errors = []
        self.stats = {
            "dirs_created": 0,
            "files_moved": 0,
            "includes_updated": 0,
            "errors": 0
        }
    
    def log(self, message: str, level: str = "INFO"):
        """Log a message with timestamp and level"""
        prefix = f"[{level}]"
        full_message = f"{prefix} {message}"
        print(full_message)
        self.operations_log.append(full_message)
    
    def check_git_status(self) -> bool:
        """Check if we're in a git repo and working directory is clean"""
        try:
            # Check if in git repo
            subprocess.run(
                ["git", "rev-parse", "--git-dir"],
                cwd=self.base_dir,
                check=True,
                capture_output=True
            )
            
            # Check if working directory is clean
            result = subprocess.run(
                ["git", "status", "--porcelain"],
                cwd=self.base_dir,
                capture_output=True,
                text=True,
                check=True
            )
            
            if result.stdout.strip():
                self.log("Working directory is not clean. Commit or stash changes first.", "ERROR")
                return False
            
            self.log("Git repository check passed", "SUCCESS")
            return True
            
        except subprocess.CalledProcessError:
            self.log("Not in a git repository or git command failed", "ERROR")
            return False
    
    def create_directory_structure(self):
        """Create all new directories needed for reorganization"""
        self.log("Creating new directory structure...")
        
        # Get unique target directories
        target_dirs = set(DIRECTORY_MAPPING.values())
        
        for target_dir in sorted(target_dirs):
            full_path = self.base_dir / target_dir
            
            if full_path.exists():
                self.log(f"Directory already exists: {target_dir}", "SKIP")
                continue
            
            if self.dry_run:
                self.log(f"Would create: {target_dir}", "DRY-RUN")
            else:
                full_path.mkdir(parents=True, exist_ok=True)
                self.log(f"Created: {target_dir}", "SUCCESS")
                self.stats["dirs_created"] += 1
    
    def move_directory(self, source: str, target: str) -> bool:
        """Move a directory using git mv"""
        src_path = self.base_dir / source
        tgt_path = self.base_dir / target
        
        if not src_path.exists():
            self.log(f"Source does not exist: {source}", "ERROR")
            self.errors.append(f"Missing source: {source}")
            self.stats["errors"] += 1
            return False
        
        # Ensure target parent directory exists
        tgt_path.parent.mkdir(parents=True, exist_ok=True)
        
        if self.dry_run:
            self.log(f"Would move: {source} → {target}", "DRY-RUN")
            return True
        
        try:
            # Use git mv to preserve history
            subprocess.run(
                ["git", "mv", str(src_path), str(tgt_path)],
                cwd=self.base_dir,
                check=True,
                capture_output=True
            )
            self.log(f"Moved: {source} → {target}", "SUCCESS")
            self.stats["files_moved"] += 1
            return True
            
        except subprocess.CalledProcessError as e:
            self.log(f"Failed to move {source}: {e}", "ERROR")
            self.errors.append(f"Failed: {source} → {target}")
            self.stats["errors"] += 1
            return False
    
    def find_all_source_files(self) -> List[Path]:
        """Find all .c and .h files in the directory tree"""
        source_files = []
        for ext in ["*.c", "*.h"]:
            source_files.extend(self.base_dir.rglob(ext))
        return source_files
    
    def update_include_path(self, old_path: str, new_path: str) -> Tuple[str, str]:
        """Generate old and new include path patterns"""
        # Convert directory path to include pattern
        # e.g., "fire_rendering" → "fire_rendering/"
        old_include = old_path.replace("/", "/")
        new_include = new_path.replace("/", "/")
        
        return old_include, new_include
    
    def update_includes_in_file(self, file_path: Path) -> int:
        """Update #include statements in a single file"""
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                content = f.read()
            
            original_content = content
            updates_count = 0
            
            # Update includes for each mapping
            for old_dir, new_dir in DIRECTORY_MAPPING.items():
                # Pattern to match #include statements
                # Matches: #include "3d_rendering/old_path/..."
                # Or: #include <3d_rendering/old_path/...>
                
                old_pattern = f"3d_rendering/{old_dir}/"
                new_pattern = f"3d_rendering/{new_dir}/"
                
                if old_pattern in content:
                    content = content.replace(old_pattern, new_pattern)
                    updates_count += content.count(new_pattern) - original_content.count(new_pattern)
            
            if content != original_content:
                if not self.dry_run:
                    with open(file_path, 'w', encoding='utf-8') as f:
                        f.write(content)
                    self.log(f"Updated {updates_count} includes in: {file_path.relative_to(self.base_dir)}", "SUCCESS")
                else:
                    self.log(f"Would update {updates_count} includes in: {file_path.relative_to(self.base_dir)}", "DRY-RUN")
                
                self.stats["includes_updated"] += updates_count
                return updates_count
            
            return 0
            
        except Exception as e:
            self.log(f"Error updating includes in {file_path}: {e}", "ERROR")
            self.errors.append(f"Include update failed: {file_path}")
            self.stats["errors"] += 1
            return 0
    
    def update_all_includes(self):
        """Update #include statements in all source files"""
        self.log("Scanning for source files to update includes...")
        source_files = self.find_all_source_files()
        self.log(f"Found {len(source_files)} source files")
        
        total_updates = 0
        for file_path in source_files:
            updates = self.update_includes_in_file(file_path)
            total_updates += updates
        
        self.log(f"Total include updates: {total_updates}", "SUCCESS")
    
    def generate_mapping_file(self, output_path: Path):
        """Generate a JSON mapping file for reference"""
        mapping_data = {
            "reorganization_date": "2026-01-06",
            "base_directory": str(self.base_dir),
            "mappings": DIRECTORY_MAPPING,
            "statistics": {
                "total_mappings": len(DIRECTORY_MAPPING),
                "target_directories": len(set(DIRECTORY_MAPPING.values()))
            }
        }
        
        if self.dry_run:
            self.log(f"Would generate mapping file: {output_path}", "DRY-RUN")
            print("\nMapping preview:")
            print(json.dumps(mapping_data, indent=2))
            with open(output_path, 'w') as f:
                json.dump(mapping_data, f, indent=2)
            self.log(f"Generated mapping file: {output_path}", "SUCCESS")
    
    def execute_reorganization(self):
        """Execute the full reorganization process"""
        self.log("="*60)
        self.log("Starting 3D Rendering Directory Reorganization")
        self.log(f"Mode: {'DRY-RUN' if self.dry_run else 'EXECUTE'}")
        self.log("="*60)
        
        # Step 1: Check git status
        if not self.dry_run and not self.check_git_status():
            self.log("Aborting due to git status check failure", "ERROR")
            return False
        
        # Step 2: Create new directory structure
        self.create_directory_structure()
        
        # Step 3: Move directories
        self.log("\nMoving directories...")
        for old_dir, new_dir in DIRECTORY_MAPPING.items():
            self.move_directory(old_dir, new_dir)
        
        # Step 4: Update includes
        self.log("\nUpdating #include paths...")
        self.update_all_includes()
        
        # Step 5: Print summary
        self.print_summary()
        
        return len(self.errors) == 0
    
    def print_summary(self):
        """Print summary statistics"""
        self.log("\n" + "="*60)
        self.log("Reorganization Summary")
        self.log("="*60)
        self.log(f"Directories created: {self.stats['dirs_created']}")
        self.log(f"Files/directories moved: {self.stats['files_moved']}")
        self.log(f"Include paths updated: {self.stats['includes_updated']}")
        self.log(f"Errors: {self.stats['errors']}")
        
        if self.errors:
            self.log("\nErrors encountered:")
            for error in self.errors:
                self.log(f"  - {error}", "ERROR")
        
        if self.dry_run:
            self.log("\nThis was a DRY-RUN. No changes were made.", "INFO")
            self.log("Run with --execute to perform the reorganization.", "INFO")


def main():
    parser = argparse.ArgumentParser(
        description="Reorganize 3D rendering directory structure"
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        default=True,
        help="Preview changes without executing (default)"
    )
    parser.add_argument(
        "--execute",
        action="store_true",
        help="Execute the reorganization (overrides --dry-run)"
    )
    parser.add_argument(
        "--generate-mapping",
        action="store_true",
        help="Only generate the mapping file"
    )
    parser.add_argument(
        "--base-dir",
        type=str,
        default=None,
        help="Base directory (default: script location)"
    )
    
    args = parser.parse_args()
    
    # Determine dry-run mode
    dry_run = not args.execute
    
    # Determine base directory
    base_dir = Path(args.base_dir) if args.base_dir else SCRIPT_DIR
    
    # Create script instance
    script = ReorganizationScript(base_dir=base_dir, dry_run=dry_run)
    
    # Execute based on mode
    if args.generate_mapping:
        mapping_output = base_dir / "reorganization_mapping.json"
        script.generate_mapping_file(mapping_output)
    else:
        success = script.execute_reorganization()
        
        # Generate mapping file on success
        if success:
            mapping_output = base_dir / "reorganization_mapping.json"
            script.generate_mapping_file(mapping_output)
        
        sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()
