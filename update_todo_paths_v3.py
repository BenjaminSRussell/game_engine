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
    if not file_path or file_path.strip() == '':
        return False
    full_path = Path("/Users/benjaminrussell/Desktop/Minecraft v2") / file_path
    return full_path.exists()

def map_old_path_to_new(old_path):
    """Map old file paths to new infrastructure locations"""
    if not old_path or old_path.strip() == '':
        return old_path
    
    # Skip if it's already in the correct format or is a directory
    if old_path.startswith('src/engine/') or not old_path.endswith('.c'):
        return old_path
    
    # Base mappings for common patterns
    mappings = {
        # Asset system mappings - these should remain as they are since they're already correct
        r'^src/engine/assets/io/([^/]+)/([^/]+)\.c$': r'src/engine/assets/io/\1/\2.c',
        
        # Legacy paths that need updating
        r'^src/Assets/(.+)$': r'src/engine/assets/\1',
        r'^src/Runtime/(.+)$': r'src/engine/\1',
        r'^src/Editor/(.+)$': r'src/engine/editor/\1',
        r'^src/Networking/(.+)$': r'src/engine/networking/\1',
        r'^src/Game/(.+)$': r'src/engine/game/\1',
        r'^editor/(.+)$': r'src/engine/editor/\1',
        r'^assets/(.+)$': r'src/engine/assets/\1',
        
        # Specific file mappings based on memory analysis
        r'^scene_io_manager\.c$': r'src/engine/assets/io/scene/manager_01.c',
        r'^asset_export_processor\.c$': r'src/engine/assets/io/export/processor_04.c',
        r'^cache_renderer\.c$': r'src/engine/rendering/cache/renderer_03.c',
        r'^compression_renderer\.c$': r'src/engine/rendering/compression/renderer_03.c',
        r'^io_compression_renderer_03\.c$': r'src/engine/rendering/compression/renderer_03.c',
        r'^io_caching_renderer_03\.c$': r'src/engine/rendering/cache/renderer_03.c',
        r'^io_scene_manager_01\.c$': r'src/engine/assets/io/scene/manager_01.c',
        r'^asset_export_processor_04\.c$': r'src/engine/assets/io/export/processor_04.c',
        r'^cache_renderer_03\.c$': r'src/engine/rendering/cache/renderer_03.c',
        r'^basis_transcoder\.c$': r'src/engine/rendering/textures/basis_transcoder.c',
        r'^cubemap_filtering\.c$': r'src/engine/rendering/environment/cubemap_filtering.c',
        r'^global_illumination\.c$': r'src/engine/rendering/global_illumination/global_illumination.c',
        r'^ai_assisted_rendering\.c$': r'src/engine/rendering/ai_assisted_rendering.c',
        r'^advanced_animation.*\.c$': r'src/engine/animation/advanced/advanced_animation.c',
        r'^NodeGraphEditor.*\.swift$': r'src/engine/editor/node_graph/NodeGraphEditor.swift',
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
    error_count = 0
    
    try:
        with open(input_file, 'r', encoding='utf-8', newline='') as f:
            reader = csv.DictReader(f)
            fieldnames = reader.fieldnames
            
            for row_num, row in enumerate(reader, start=2):  # Start at 2 for line numbers
                try:
                    # Ensure row has all expected fields
                    clean_row = {}
                    for field in fieldnames:
                        clean_row[field] = row.get(field, '')
                    
                    old_source = clean_row.get('source', '')
                    
                    # Skip documentation sources and directories
                    if (not old_source or 
                        old_source.endswith('.md') or 
                        old_source.endswith('.h') or
                        not old_source.endswith('.c') or
                        ':' in old_source or  # Skip line-specific references
                        old_source in ['cmake', 'testing', 'ui', 'audio', 'animation', 'rendering', 'physics', 'ai', 'networking', 'assets', 'geometry', 'scripting', 'tools', 'environment', 'cinematics', 'platform', 'memory', 'docs', 'optimization']):  # Skip directories
                        updated_rows.append(clean_row)
                        unchanged_count += 1
                        continue
                    
                    # Map to new path
                    new_source = map_old_path_to_new(old_source)
                    
                    # Check if the new path exists
                    if check_file_exists(new_source):
                        clean_row['source'] = new_source
                        updated_count += 1
                        if old_source != new_source:
                            print(f"Updated: {old_source} -> {new_source}")
                    elif check_file_exists(old_source):
                        # Original path exists, keep it
                        unchanged_count += 1
                    else:
                        # Neither path exists, but still update to the new format
                        not_found_count += 1
                        if old_source != new_source:
                            print(f"NOT FOUND: {old_source} -> {new_source}")
                        clean_row['source'] = new_source
                    
                    updated_rows.append(clean_row)
                    
                except Exception as e:
                    print(f"Error processing row {row_num}: {e}")
                    error_count += 1
                    continue
        
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
        print(f"Errors encountered: {error_count}")
        
    except Exception as e:
        print(f"Fatal error: {e}")
        error_count += 1

if __name__ == "__main__":
    input_file = "/Users/benjaminrussell/Desktop/Minecraft v2/master/todo 2.csv"
    output_file = "/Users/benjaminrussell/Desktop/Minecraft v2/master/todo_updated.csv"
    
    update_todo_csv(input_file, output_file)
    print(f"\nUpdated TODO file saved to: {output_file}")
