#!/usr/bin/env python3
"""
Targeted Particle System Consolidation Script
Specifically targets actual particle system duplicates
"""

import os
import re
import sys
from pathlib import Path

# Known particle system files to analyze
PARTICLE_SYSTEMS = {
    'gpu_particle_system': [
        'src/engine/core/services/gpu_particle_system.c',
        'src/engine/core/services/particle_system_gpu.c',
        'src/engine/effects/gpu_particles/gpu_particle_system.c',
        'src/engine/effects/gpu_particles/gpu_particle_system.m'
    ],
    'particle_renderer': [
        'src/engine/core/services/particle_renderer.c',
        'src/engine/effects/particles/particle_renderer.c'
    ],
    'particle_system': [
        'src/engine/core/services/particle_system_gpu.c.disabled'
    ]
}

def analyze_file_content(filepath):
    """Analyze file content to determine if it's a real particle system"""
    if not os.path.exists(filepath):
        return False, 0
    
    try:
        with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read().lower()
        
        # Check for particle-specific keywords
        particle_keywords = [
            'particle', 'emitter', 'spawn', 'lifetime', 'velocity',
            'acceleration', 'color', 'size', 'texture', 'shader'
        ]
        
        keyword_count = sum(1 for keyword in particle_keywords if keyword in content)
        
        # Must have at least 3 particle keywords to be considered a particle system
        is_particle_system = keyword_count >= 3
        
        return is_particle_system, keyword_count
        
    except:
        return False, 0

def consolidate_particle_systems():
    """Consolidate actual particle system duplicates"""
    consolidated_count = 0
    
    for system_type, files in PARTICLE_SYSTEMS.items():
        print(f"\n=== Analyzing {system_type} ===")
        
        valid_files = []
        for filepath in files:
            is_particle, keyword_count = analyze_file_content(filepath)
            if is_particle:
                valid_files.append({
                    'path': filepath,
                    'size': os.path.getsize(filepath),
                    'keywords': keyword_count
                })
                print(f"Valid particle system: {filepath} ({keyword_count} keywords)")
            else:
                print(f"Not a particle system: {filepath}")
        
        # If we have duplicates, keep the best one
        if len(valid_files) > 1:
            # Choose best implementation (most keywords, then largest size)
            best_file = max(valid_files, key=lambda x: (x['keywords'], x['size']))
            print(f"Keeping: {best_file['path']} ({best_file['keywords']} keywords, {best_file['size']} bytes)")
            
            # Remove duplicates
            for file_info in valid_files:
                if file_info['path'] != best_file['path']:
                    try:
                        os.remove(file_info['path'])
                        print(f"Removed: {file_info['path']} ({file_info['keywords']} keywords, {file_info['size']} bytes)")
                        consolidated_count += 1
                    except Exception as e:
                        print(f"Error removing {file_info['path']}: {e}")
        elif len(valid_files) == 1:
            print(f"Single implementation found: {valid_files[0]['path']}")
        else:
            print("No valid particle systems found")
    
    return consolidated_count

def main():
    """Main consolidation function"""
    print("Analyzing particle systems for consolidation...")
    
    consolidated_count = consolidate_particle_systems()
    
    print(f"\nConsolidation complete!")
    print(f"Removed {consolidated_count} duplicate particle system files")

if __name__ == "__main__":
    main()
