#!/usr/bin/env python3
"""
Particle System Consolidation Script
Identifies and removes duplicate particle implementations
"""

import os
import re
import sys
from pathlib import Path

# Particle system patterns to identify
PARTICLE_SYSTEMS = [
    ('gpu_particle_system', 'GPU-based particle system'),
    ('particle_renderer', 'Particle rendering system'),
    ('particle_system', 'Core particle system'),
    ('particle_emitter', 'Particle emitter system'),
    ('particle_simulation', 'Particle simulation system'),
    ('gpu_particles', 'GPU particles (modern)'),
    ('svg_particles', 'SVG particle system'),
    ('puddles', 'Particle puddles'),
    ('trails', 'Particle trails'),
    ('smoke', 'Smoke particle system'),
    ('fire', 'Fire particle system'),
    ('explosions', 'Explosion particle system'),
    ('weather', 'Weather particles')
]

def analyze_particle_file(filepath):
    """Analyze a particle system file to identify its type"""
    try:
        with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read().lower()
            
        # Check for key indicators
        for system_type, description in PARTICLE_SYSTEMS:
            if system_type in content:
                return system_type, description
                
        # Check for GPU-related patterns
        if any(keyword in content for keyword in ['gpu', 'compute', 'shader', 'buffer']):
            return 'gpu_particles', 'GPU-based particle system'
            
        # Check for rendering patterns
        if any(keyword in content for keyword in ['render', 'draw', 'vertex', 'fragment']):
            return 'particle_renderer', 'Particle rendering system'
            
        # Check for simulation patterns
        if any(keyword in content for keyword in ['simulate', 'update', 'physics', 'integration']):
            return 'particle_simulation', 'Particle simulation system'
                
    except:
        pass
    
    return None, None

def find_duplicate_systems():
    """Find duplicate particle system implementations"""
    particle_files = {}
    
    # Search for particle-related files
    for root_dir in ['/Users/benjaminrussell/Desktop/Minecraft v2/src/engine/effects',
                       '/Users/benjaminrussell/Desktop/Minecraft v2/src/engine/core/services']:
        if os.path.exists(root_dir):
            for item in os.listdir(root_dir):
                if item.endswith('.c') or item.endswith('.h'):
                    filepath = os.path.join(root_dir, item)
                    system_type, description = analyze_particle_file(filepath)
                    if system_type and description:
                        if system_type not in particle_files:
                            particle_files[system_type] = []
                        particle_files[system_type].append({
                            'path': filepath,
                            'description': description,
                            'size': os.path.getsize(filepath)
                        })
    
    # Find duplicates
    duplicates = {}
    for system_type, files in particle_files.items():
        if len(files) > 1:
            duplicates[system_type] = files
    
    return duplicates

def consolidate_particle_systems(duplicates):
    """Consolidate duplicate particle systems"""
    consolidated_count = 0
    
    for system_type, files in duplicates.items():
        if len(files) <= 1:
            continue
            
        print(f"\n=== Consolidating {system_type} ===")
        print(f"Found {len(files)} duplicate implementations")
        
        # Choose the best implementation (largest file is usually most complete)
        best_file = max(files, key=lambda x: x['size'])
        print(f"Keeping: {best_file['path']} ({best_file['size']} bytes)")
        
        # Remove duplicates
        for file_info in files:
            if file_info['path'] != best_file['path']:
                try:
                    os.remove(file_info['path'])
                    print(f"Removed: {file_info['path']} ({file_info['size']} bytes)")
                    consolidated_count += 1
                except Exception as e:
                    print(f"Error removing {file_info['path']}: {e}")
    
    return consolidated_count

def main():
    """Main consolidation function"""
    if len(sys.argv) != 2:
        print("Usage: python3 consolidate_particles.py <source_directory>")
        sys.exit(1)
    
    source_dir = Path(sys.argv[1])
    if not source_dir.exists():
        print(f"Directory {source_dir} does not exist")
        sys.exit(1)
    
    print("Analyzing particle systems for duplicates...")
    duplicates = find_duplicate_systems()
    
    if not duplicates:
        print("No duplicate particle systems found")
        return
    
    total_duplicates = sum(len(files) for files in duplicates.values())
    print(f"Found {len(duplicates)} system types with duplicates")
    print(f"Total duplicate files: {total_duplicates}")
    
    consolidated_count = consolidate_particle_systems(duplicates)
    
    print(f"\nConsolidation complete!")
    print(f"Removed {consolidated_count} duplicate files")
    print(f"Kept {len(duplicates)} primary implementations")

if __name__ == "__main__":
    main()
