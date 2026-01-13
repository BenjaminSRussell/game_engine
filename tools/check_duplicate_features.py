#!/usr/bin/env python3
"""
Pre-merge duplicate detection script.
Checks if a branch's features overlap with existing code.
"""

import subprocess
import sys
import re
from pathlib import Path

def get_branch_files(branch):
    """Get list of new/modified files in branch"""
    result = subprocess.run(
        ['git', 'diff', '--name-status', f'main..origin/{branch}'],
        capture_output=True, text=True
    )
    if result.returncode != 0:
        return []
    return [line.split('\t') for line in result.stdout.strip().split('\n') if line]

def search_similar_files(filename):
    """Search for similarly named files in codebase"""
    base = Path(filename).stem
    if not base or len(base) < 4: return []
    
    result = subprocess.run(
        ['find', 'src', '-name', f'*{base}*', '-type', 'f'],
        capture_output=True, text=True
    )
    candidates = [f for f in result.stdout.strip().split('\n') if f and f != filename]
    return candidates

def search_similar_functions(branch):
    """Extract function names from branch and search for duplicates"""
    # Get new function definitions
    result = subprocess.run(
        ['git', 'diff', f'main...origin/{branch}'],
        capture_output=True, text=True
    )
    if result.returncode != 0:
        return {}
    
    # Simple regex for C function definitions
    # Matches: void/int/type func_name(args) {
    func_pattern = r'^\+[a-z_0-9]+\s+\*?([a-z_0-9]+)\s*\('
    new_functions = set()
    
    for line in result.stdout.split('\n'):
        match = re.match(func_pattern, line, re.IGNORECASE)
        if match:
            func = match.group(1)
            # Filter common words
            if func not in ['if', 'while', 'for', 'switch', 'main']:
                new_functions.add(func)
    
    # Search for each function in main branch
    duplicates = {}
    for func in new_functions:
        result = subprocess.run(
            ['git', 'grep', '-n', func, 'main', '--', '*.c', '*.h'],
            capture_output=True, text=True
        )
        if result.returncode == 0:
            duplicates[func] = result.stdout.strip().split('\n')[:3]  # First 3 matches
    
    return duplicates

def main():
    if len(sys.argv) < 2:
        print("Usage: check_duplicate_features.py <branch-name>")
        sys.exit(1)
    
    branch = sys.argv[1]
    print(f"Checking branch: {branch}")
    print("=" * 60)
    
    # Check files
    files = get_branch_files(branch)
    print(f"\nModified files: {len(files)}")
    
    duplicates_found = False
    
    for item in files:
        if len(item) < 2: continue
        status, filepath = item[0], item[1]
        
        if status == 'A':  # New file
            similar = search_similar_files(filepath)
            if similar:
                print(f"\n⚠️  Similar files found for {filepath}:")
                for s in similar:
                    print(f"   - {s}")
                duplicates_found = True
    
    # Check functions
    print("\nSearching for duplicate functions...")
    func_dups = search_similar_functions(branch)
    
    if func_dups:
        print("\n⚠️  Potentially duplicate functions:")
        for func, locations in func_dups.items():
            print(f"\n  {func}:")
            for loc in locations:
                print(f"    - {loc}")
        duplicates_found = True
    
    print("\n" + "=" * 60)
    
    if duplicates_found:
        print("❌ Potential duplicates found - manual review recommended")
        sys.exit(1)
    else:
        print("✅ No obvious duplicates detected")
        sys.exit(0)

if __name__ == "__main__":
    main()
