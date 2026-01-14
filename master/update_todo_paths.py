#!/usr/bin/env python3
import csv
import re
from pathlib import Path

def update_todo_paths():
    # Read current todo.csv
    with open('/Users/benjaminrussell/Desktop/Minecraft v2/master/todo.csv', 'r') as f:
        lines = f.readlines()
    
    # Parse header and data
    if not lines:
        return
    
    header = lines[0].strip().split(',')
    todos = []
    
    for line in lines[1:]:
        fields = line.strip().split(',')
        # Ensure we have exactly 6 fields
        while len(fields) < 6:
            fields.append('')
        todo = {
            'id': fields[0],
            'type': fields[1],
            'category': fields[2],
            'source': fields[3],
            'description': fields[4],
            'status': fields[5]
        }
        todos.append(todo)
    
    # Path mapping for infrastructure refactoring
    path_mappings = {
        # Old src paths -> new Source/Runtime paths
        'src/engine/': 'Source/Runtime/Engine/',
        'src/core/': 'Source/Runtime/Core/',
        'src/renderer/': 'Source/Runtime/Renderer/',
        'src/physics/': 'Source/Runtime/Physics/',
        'src/audio/': 'Source/Runtime/Audio/',
        'src/animation/': 'Source/Runtime/Animation/',
        'src/ai/': 'Source/Runtime/AI/',
        'src/networking/': 'Source/Runtime/Networking/',
        'src/input/': 'Source/Runtime/Input/',
        'src/ui/': 'Source/Runtime/UI/',
        'src/scene/': 'Source/Runtime/Scene/',
        'src/ecs/': 'Source/Runtime/ECS/',
        'src/rhi/': 'Source/Runtime/RHI/',
        'src/pcg/': 'Source/Runtime/PCG/',
        'src/ml/': 'Source/Runtime/ML/',
        'src/procedural/': 'Source/Runtime/Procedural/',
        
        # Include paths
        'include/engine/': 'include/Engine/',
        'include/core/': 'include/Core/',
        'include/renderer/': 'include/Renderer/',
        'include/physics/': 'include/Physics/',
        'include/audio/': 'include/Audio/',
        'include/animation/': 'include/Animation/',
        'include/ai/': 'include/AI/',
        'include/networking/': 'include/Networking/',
        'include/input/': 'include/Input/',
        'include/ui/': 'include/UI/',
        'include/scene/': 'include/Scene/',
        'include/ecs/': 'include/ECS/',
        'include/rhi/': 'include/RHI/',
        
        # Editor paths
        'editor/': 'Source/Editor/',
        
        # Game paths
        'game/': 'Source/Game/',
    }
    
    updated_count = 0
    
    # Update each todo entry
    for todo in todos:
        description = todo['description']
        original_desc = description
        
        # Update file paths in descriptions
        for old_path, new_path in path_mappings.items():
            if old_path in description:
                description = description.replace(old_path, new_path)
                updated_count += 1
        
        # Update source field if it contains file paths
        source = todo['source']
        for old_path, new_path in path_mappings.items():
            if old_path in source:
                source = source.replace(old_path, new_path)
                updated_count += 1
        
        todo['description'] = description
        todo['source'] = source
    
    # Write updated todos back
    with open('/Users/benjaminrussell/Desktop/Minecraft v2/master/todo_updated.csv', 'w') as f:
        # Write header
        f.write('id,type,category,source,description,status\n')
        # Write data
        for todo in todos:
            # Escape commas and quotes in fields
            fields = []
            for key in ['id', 'type', 'category', 'source', 'description', 'status']:
                value = todo[key]
                if ',' in value or '"' in value or '\n' in value:
                    value = '"' + value.replace('"', '""') + '"'
                fields.append(value)
            f.write(','.join(fields) + '\n')
    
    print(f"Updated {updated_count} path references")
    print(f"Total todos: {len(todos)}")
    print("Output written to todo_updated.csv")

if __name__ == "__main__":
    update_todo_paths()
