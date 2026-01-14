#!/usr/bin/env python3
"""
Error Handling Standardization Script
Replaces inconsistent error handling patterns with the standardized error handling system
"""

import os
import re
import sys
from pathlib import Path

# Patterns to replace and their standardized equivalents
ERROR_PATTERNS = [
    # Replace fprintf(stderr, "message") with REPORT_ERROR
    (
        r'fprintf\s*\(\s*stderr\s*,\s*"([^"]*)"\s*\)\s*;',
        r'REPORT_ERROR(ERROR_OPERATION_FAILED, "\1");'
    ),
    
    # Replace fprintf(stderr, "message %s", arg) with REPORT_ERROR
    (
        r'fprintf\s*\(\s*stderr\s*,\s*"([^"]*%[^"]*)"\s*,\s*([^)]+)\s*\)\s*;',
        r'REPORT_ERROR(ERROR_OPERATION_FAILED, "\1", \2);'
    ),
    
    # Replace printf("ERROR: ...) with REPORT_ERROR
    (
        r'printf\s*\(\s*"ERROR:\s*([^"]*)"\s*\)\s*;',
        r'REPORT_ERROR(ERROR_OPERATION_FAILED, "\1");'
    ),
    
    # Replace printf("ERROR: %s", arg) with REPORT_ERROR
    (
        r'printf\s*\(\s*"ERROR:\s*([^"]*%[^"]*)"\s*,\s*([^)]+)\s*\)\s*;',
        r'REPORT_ERROR(ERROR_OPERATION_FAILED, "\1", \2);'
    ),
    
    # Replace printf("Error: ...) with REPORT_ERROR
    (
        r'printf\s*\(\s*"Error:\s*([^"]*)"\s*\)\s*;',
        r'REPORT_ERROR(ERROR_OPERATION_FAILED, "\1");'
    ),
    
    # Replace printf("Error: %s", arg) with REPORT_ERROR
    (
        r'printf\s*\(\s*"Error:\s*([^"]*%[^"]*)"\s*,\s*([^)]+)\s*\)\s*;',
        r'REPORT_ERROR(ERROR_OPERATION_FAILED, "\1", \2);'
    ),
]

def should_skip_file(filepath):
    """Skip files that already use standardized error handling"""
    try:
        with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()
            # If file already includes error_handling.h and uses REPORT_ERROR, skip it
            if '#include "error_handling.h"' in content and 'REPORT_ERROR(' in content:
                return True
            # Skip the error handling implementation files themselves
            if 'error_handling' in filepath.name:
                return True
    except:
        pass
    return False

def standardize_file(filepath):
    """Standardize error handling in a single file"""
    try:
        with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()
        
        original_content = content
        
        # Add error_handling.h include if not present and file needs standardization
        if '#include "error_handling.h"' not in content:
            # Find the last include statement
            includes = re.findall(r'#include\s*"[^"]*"', content)
            if includes:
                last_include = includes[-1]
                insert_pos = content.find(last_include) + len(last_include)
                content = content[:insert_pos] + '\n#include "error_handling.h"' + content[insert_pos:]
            else:
                # Add after initial comments
                lines = content.split('\n')
                insert_line = 0
                for i, line in enumerate(lines):
                    if line.strip() and not line.strip().startswith('//') and not line.strip().startswith('/*'):
                        insert_line = i
                        break
                lines.insert(insert_line, '#include "error_handling.h"')
                content = '\n'.join(lines)
        
        # Apply error handling patterns
        for pattern, replacement in ERROR_PATTERNS:
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
    """Main standardization function"""
    if len(sys.argv) != 2:
        print("Usage: python3 standardize_errors.py <source_directory>")
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
            print(f"Skipping {filepath} (already standardized)")
            continue
        
        print(f"Processing {filepath}...", end=' ')
        if standardize_file(filepath):
            print("MODIFIED")
            modified_files += 1
        else:
            print("no changes")
    
    print(f"\nStandardization complete!")
    print(f"Processed: {processed_files} files")
    print(f"Modified: {modified_files} files")
    print(f"Skipped: {processed_files - modified_files} files")

if __name__ == "__main__":
    main()
