#!/usr/bin/env python3
"""
Script to update TODO file paths in the CSV to match new infrastructure
"""

import csv
import os
import re
from pathlib import Path

def check_file_exists(file_path):
    """Check if a file exists in the new infrastructure"""
    full_path = Path("/Users/benjaminrussell/Desktop/Minecraft v2") / file_path
    return full_path.exists()

def map_old_path_to_new(old_path):
    """Map old file paths to new infrastructure locations"""
    # Base mappings for common patterns
    mappings = {
        # Asset system mappings
        r'src/engine/assets/io/([^/]+)/manager_01\.c': r'src/engine/assets/io/\1/manager_01.c',
        r'src/engine/assets/io/([^/]+)/processor_04\.c': r'src/engine/assets/io/\1/processor_04.c',
        r'src/engine/assets/io/([^/]+)/renderer_03\.c': r'src/engine/assets/io/\1/renderer_03.c',
        
        # Rendering system mappings
        r'src/([^/]+)/rendering/([^/]+)\.c': r'src/engine/rendering/\2.c',
        r'src/([^/]+)/([^/]+)/rendering/([^/]+)\.c': r'src/engine/rendering/\3.c',
        
        # Physics system mappings
        r'src/Runtime/Physics/([^/]+)/([^/]+)\.c': r'src/engine/physics/\1/\2.c',
        
        # AI system mappings
        r'src/Runtime/AI/([^/]+)/([^/]+)\.c': r'src/engine/ai/\1/\2.c',
        
        # Animation system mappings
        r'src/Runtime/Animation/([^/]+)/([^/]+)\.c': r'src/engine/animation/\1/\2.c',
        
        # Editor mappings
        r'editor/([^/]+)/([^/]+)\.c': r'src/engine/editor/\1/\2.c',
        
        # Network mappings
        r'src/Networking/([^/]+)\.c': r'src/engine/networking/\1.c',
    }
    
    new_path = old_path
    for pattern, replacement in mappings.items():
        if re.match(pattern, old_path):
            new_path = re.sub(pattern, replacement, old_path)
            break
    
    return new_path

def update_todo_csv(input_file, output_file):
    """Update the TODO CSV file with new paths"""
    updated_rows = []
    unchanged_count = 0
    updated_count = 0
    not_found_count = 0
    
    with open(input_file, 'r', encoding='utf-8') as f:
        reader = csv.DictReader(f)
        fieldnames = reader.fieldnames
        
        for row in reader:
            old_source = row['source']
            
            # Skip documentation sources
            if old_source.endswith('.md'):
                updated_rows.append(row)
                unchanged_count += 1
                continue
            
            # Map to new path
            new_source = map_old_path_to_new(old_source)
            
            # Check if the new path exists
            if check_file_exists(new_source):
                row['source'] = new_source
                updated_count += 1
                print(f"Updated: {old_source} -> {new_source}")
            elif check_file_exists(old_source):
                # Original path still exists, keep it
                unchanged_count += 1
            else:
                # Neither path exists, mark as potentially missing
                not_found_count += 1
                print(f"NOT FOUND: {old_source} (tried: {new_source})")
                # Still update to the new path format
                row['source'] = new_source
            
            updated_rows.append(row)
    
    # Write updated CSV
    with open(output_file, 'w', encoding='utf-8', newline='') as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(updated_rows)
    
    print(f"\nUpdate Summary:")
    print(f"Total rows processed: {len(updated_rows)}")
    print(f"Paths updated: {updated_count}")
    print(f"Paths unchanged: {unchanged_count}")
    print(f"Paths not found: {not_found_count}")

if __name__ == "__main__":
    input_file = "/Users/benjaminrussell/Desktop/Minecraft v2/master/todo 2.csv"
    output_file = "/Users/benjaminrussell/Desktop/Minecraft v2/master/todo_updated.csv"
    
    update_todo_csv(input_file, output_file)
    print(f"\nUpdated TODO file saved to: {output_file}")
