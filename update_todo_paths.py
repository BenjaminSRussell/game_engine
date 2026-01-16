#!/usr/bin/env python3
"""
Script to update TODO CSV file paths to reflect new infrastructure structure.
This maps old file paths to new locations based on current codebase structure.
"""

import csv
import re
import os
from pathlib import Path

def create_path_mapping():
    """Create mapping between old and new file paths based on current infrastructure."""
    
    # Base directory to check against
    base_dir = Path("/Users/benjaminrussell/Desktop/Minecraft v2")
    
    # Mapping of old patterns to new patterns
    path_mappings = {
        # Audio system paths
        "src/engine/audio/": "src/engine/audio/",
        
        # Asset I/O paths - these have been reorganized
        "src/engine/assets/io/async/": "src/engine/assets/io/async/",
        "src/engine/assets/io/bundling/": "src/engine/assets/io/bundling/",
        "src/engine/assets/io/caching/": "src/engine/assets/io/caching/",
        "src/engine/assets/io/compression/": "src/engine/assets/io/compression/",
        "src/engine/assets/io/export/": "src/engine/assets/io/export/",
        "src/engine/assets/io/formats/": "src/engine/assets/io/formats/",
        "src/engine/assets/io/import/": "src/engine/assets/io/import/",
        "src/engine/assets/io/scene/": "src/engine/assets/io/scene/",
        "src/engine/assets/io/serialization/": "src/engine/assets/io/serialization/",
        "src/engine/assets/io/streaming/": "src/engine/assets/io/streaming/",
        
        # Alternative Assets path (some files may be here)
        "src/Assets/io/async/": "src/Assets/io/async/",
        "src/Assets/io/bundling/": "src/Assets/io/bundling/",
        "src/Assets/io/caching/": "src/Assets/io/caching/",
        "src/Assets/io/compression/": "src/Assets/io/compression/",
        "src/Assets/io/export/": "src/Assets/io/export/",
        "src/Assets/io/formats/": "src/Assets/io/formats/",
        "src/Assets/io/import/": "src/Assets/io/import/",
        "src/Assets/io/scene/": "src/Assets/io/scene/",
        "src/Assets/io/serialization/": "src/Assets/io/serialization/",
        "src/Assets/io/streaming/": "src/Assets/io/streaming/",
        
        # Core engine paths
        "src/engine/core/": "src/engine/core/",
        "src/engine/renderer/": "src/Renderer/",
        "src/engine/physics/": "src/Runtime/Physics/",
        "src/engine/ai/": "src/Runtime/AI/",
        "src/engine/animation/": "src/Runtime/Animation/",
        "src/engine/audio/": "src/audio/",
        
        # Editor paths
        "src/engine/editor/": "src/Editor/",
        
        # Networking paths
        "src/engine/networking/": "src/Networking/",
        
        # UI paths
        "src/engine/ui/": "src/UI/",
        
        # Scripting paths
        "src/engine/scripting/": "src/Scripting/",
        
        # RHI (Rendering Hardware Interface) paths
        "src/engine/rhi/": "src/RHI/",
        
        # Game logic paths
        "src/engine/game/": "src/Game/",
    }
    
    return path_mappings

def verify_path_exists(file_path, base_dir):
    """Check if a file path exists in current codebase."""
    full_path = base_dir / file_path
    return full_path.exists()

def find_alternative_path(old_path, base_dir):
    """Try to find alternative location for a file that doesn't exist at expected path."""
    filename = Path(old_path).name
    
    # Common directories to search
    search_dirs = [
        "src/engine/",
        "src/Assets/",
        "src/Runtime/",
        "src/Renderer/",
        "src/Editor/",
        "src/UI/",
        "src/Networking/",
        "src/Scripting/",
        "src/RHI/",
        "src/Game/",
        "src/audio/",
        "src/core/",
        "src/debug/",
        "src/vendor/",
    ]
    
    for search_dir in search_dirs:
        for found_file in (base_dir / search_dir).rglob(filename):
            if found_file.is_file():
                return str(found_file.relative_to(base_dir))
    
    return None

def map_old_path_to_new(old_path):
    """Map old file paths to new infrastructure locations"""
    path_mappings = create_path_mapping()
    
    # Apply direct mappings first
    new_path = old_path
    for old_pattern, new_pattern in path_mappings.items():
        if old_path.startswith(old_pattern):
            new_path = new_pattern + old_path[len(old_pattern):]
            break
    
    return new_path

def update_todo_csv(input_file, output_file):
    """Update TODO CSV file with corrected file paths."""
    
    base_dir = Path("/Users/benjaminrussell/Desktop/Minecraft v2")
    path_mappings = create_path_mapping()
    
    updated_rows = []
    updates_made = 0
    files_not_found = []
    
    with open(input_file, 'r', encoding='utf-8') as csvfile:
        reader = csv.DictReader(csvfile)
        fieldnames = reader.fieldnames
        
        for row in reader:
            original_description = row['description']
            original_source = row['source']
            
            # Skip if description doesn't contain a file path
            if not ('src/' in original_description and '.c' in original_description):
                updated_rows.append(row)
                continue
            
            # Extract file path from description
            # Look for patterns like "src/engine/audio/audio_system.c"
            file_path_match = re.search(r'(src/[^\s\)]+\.c)', original_description)
            
            if not file_path_match:
                updated_rows.append(row)
                continue
            
            old_file_path = file_path_match.group(1)
            new_file_path = old_file_path
            
            # Apply path mappings
            for old_pattern, new_pattern in path_mappings.items():
                if old_file_path.startswith(old_pattern):
                    new_file_path = new_pattern + old_file_path[len(old_pattern):]
                    break
            
            # Verify new path exists
            if verify_path_exists(new_file_path, base_dir):
                # Update description with new path
                updated_description = original_description.replace(old_file_path, new_file_path)
                row['description'] = updated_description
                updates_made += 1
            else:
                # Try to find alternative location
                alternative_path = find_alternative_path(old_file_path, base_dir)
                if alternative_path:
                    updated_description = original_description.replace(old_file_path, alternative_path)
                    row['description'] = updated_description
                    updates_made += 1
                    print(f"Found alternative for {old_file_path}: {alternative_path}")
                else:
                    files_not_found.append(old_file_path)
                    print(f"Could not find location for: {old_file_path}")
            
            updated_rows.append(row)
    
    # Write updated CSV
    with open(output_file, 'w', newline='', encoding='utf-8') as csvfile:
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
        writer.writeheader()
        
        # Filter out any rows with None values in fields
        clean_rows = []
        for row in updated_rows:
            clean_row = {}
            for field in fieldnames:
                clean_row[field] = row.get(field, '')
            clean_rows.append(clean_row)
        
        writer.writerows(clean_rows)
    
    print(f"\nUpdate Summary:")
    print(f"- Total updates made: {updates_made}")
    print(f"- Files not found: {len(files_not_found)}")
    
    if files_not_found:
        print(f"\nFiles that could not be located:")
        for file_path in files_not_found[:10]:  # Show first 10
            print(f"  - {file_path}")
        if len(files_not_found) > 10:
            print(f"  ... and {len(files_not_found) - 10} more")

def main():
    """Main function to run the TODO path update process."""
    
    input_file = "/Users/benjaminrussell/Desktop/Minecraft v2/master/todo 2.csv"
    output_file = "/Users/benjaminrussell/Desktop/Minecraft v2/master/todo 2_updated.csv"
    
    print("Updating TODO file paths to reflect new infrastructure...")
    print(f"Input file: {input_file}")
    print(f"Output file: {output_file}")
    
    update_todo_csv(input_file, output_file)
    
    print(f"\nPath update complete!")
    print(f"Updated file saved as: {output_file}")
    print("\nPlease review the updated file and replace the original if satisfactory.")

if __name__ == "__main__":
    main()
