#!/usr/bin/env python3
"""
Memory Management Consolidation Script
Replaces direct malloc/free calls with the unified memory allocator system
"""

import os
import re
import sys
from pathlib import Path

# Memory allocation patterns to replace
MEMORY_PATTERNS = [
    # Replace malloc(size) with UNIFIED_ALLOC(size, strategy, flags)
    (
        r'(\w+)\s*=\s*malloc\s*\(\s*([^)]+)\s*\)\s*;',
        r'\1 = UNIFIED_ALLOC(\2, MEMORY_STRATEGY_DEFAULT, MEMORY_FLAG_TRACK);'
    ),
    
    # Replace calloc(count, size) with UNIFIED_ALLOC
    (
        r'(\w+)\s*=\s*calloc\s*\(\s*([^,]+)\s*,\s*([^)]+)\s*\)\s*;',
        r'\1 = UNIFIED_ALLOC(\2 * \3, MEMORY_STRATEGY_DEFAULT, MEMORY_FLAG_ZERO | MEMORY_FLAG_TRACK);'
    ),
    
    # Replace realloc(ptr, size) with UNIFIED_REALLOC
    (
        r'(\w+)\s*=\s*realloc\s*\(\s*([^,]+)\s*,\s*([^)]+)\s*\)\s*;',
        r'\1 = UNIFIED_REALLOC(\2, \3, MEMORY_FLAG_TRACK);'
    ),
    
    # Replace free(ptr) with UNIFIED_FREE
    (
        r'free\s*\(\s*([^)]+)\s*\)\s*;',
        r'UNIFIED_FREE(\1);'
    ),
    
    # Replace malloc without assignment
    (
        r'malloc\s*\(\s*([^)]+)\s*\)\s*;',
        r'UNIFIED_ALLOC(\1, MEMORY_STRATEGY_DEFAULT, MEMORY_FLAG_TRACK);'
    ),
    
    # Replace calloc without assignment
    (
        r'calloc\s*\(\s*([^,]+)\s*,\s*([^)]+)\s*\)\s*;',
        r'UNIFIED_ALLOC(\1 * \2, MEMORY_STRATEGY_DEFAULT, MEMORY_FLAG_ZERO | MEMORY_FLAG_TRACK);'
    ),
    
    # Replace realloc without assignment
    (
        r'realloc\s*\(\s*([^,]+)\s*,\s*([^)]+)\s*\)\s*;',
        r'UNIFIED_REALLOC(\1, \2, MEMORY_FLAG_TRACK);'
    ),
]

def should_skip_file(filepath):
    """Skip files that already use unified memory allocator"""
    try:
        with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()
            # If file already includes unified_memory_allocator.h and uses UNIFIED_ALLOC, skip it
            if 'unified_memory_allocator.h' in content and 'UNIFIED_ALLOC(' in content:
                return True
            # Skip the unified memory allocator files themselves
            if 'unified_memory_allocator' in filepath.name:
                return True
            # Skip system headers and standard library files
            if any(x in filepath.name.lower() for x in ['test', 'stub', 'legacy']):
                return True
    except:
        pass
    return False

def add_unified_include(content, filepath):
    """Add unified memory allocator include if needed"""
    if 'unified_memory_allocator.h' in content:
        return content
    
    # Find the last include statement
    includes = re.findall(r'#include\s*"[^"]*"', content)
    if includes:
        last_include = includes[-1]
        insert_pos = content.find(last_include) + len(last_include)
        content = content[:insert_pos] + '\n#include "unified_memory_allocator.h"' + content[insert_pos:]
    else:
        # Add after initial comments
        lines = content.split('\n')
        insert_line = 0
        for i, line in enumerate(lines):
            if line.strip() and not line.strip().startswith('//') and not line.strip().startswith('/*'):
                insert_line = i
                break
        lines.insert(insert_line, '#include "unified_memory_allocator.h"')
        content = '\n'.join(lines)
    
    return content

def consolidate_file(filepath):
    """Consolidate memory management in a single file"""
    try:
        with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()
        
        original_content = content
        
        # Add unified memory allocator include if needed
        content = add_unified_include(content, filepath)
        
        # Apply memory consolidation patterns
        for pattern, replacement in MEMORY_PATTERNS:
            content = re.sub(pattern, replacement, content, flags=re.MULTILINE)
        
        # Only write if changes were made
        if content != original_content:
            with open(filepath, 'w', encoding='utf-8') as f:
                f.write(content)
            return True
        
    except Exception as e:
        print(f"Error processing {filepath}: {e}")
        return False
    
    return False

def main():
    """Main consolidation function"""
    if len(sys.argv) != 2:
        print("Usage: python3 consolidate_memory.py <source_directory>")
        sys.exit(1)
    
    source_dir = Path(sys.argv[1])
    if not source_dir.exists():
        print(f"Directory {source_dir} does not exist")
        sys.exit(1)
    
    # Find all C files
    c_files = list(source_dir.rglob("*.c"))
    
    processed_files = 0
    modified_files = 0
    
    print(f"Found {len(c_files)} C files to process...")
    
    for filepath in c_files:
        processed_files += 1
        
        if should_skip_file(filepath):
            print(f"Skipping {filepath} (already consolidated)")
            continue
        
        print(f"Processing {filepath}...", end=' ')
        if consolidate_file(filepath):
            print("MODIFIED")
            modified_files += 1
        else:
            print("no changes")
    
    print(f"\nMemory consolidation complete!")
    print(f"Processed: {processed_files} files")
    print(f"Modified: {modified_files} files")
    print(f"Skipped: {processed_files - modified_files} files")

if __name__ == "__main__":
    main()
