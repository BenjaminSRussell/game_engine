#!/usr/bin/env python3
"""
Memory Management Migration Script
Consolidates all memory allocation calls to use the unified memory system
"""

import os
import re
import sys
from pathlib import Path

class MemoryMigrator:
    def __init__(self, root_dir):
        self.root_dir = Path(root_dir)
        self.migrations = {
            # Standard library functions
            r'\bmalloc\s*\(': 'UNIFIED_ALLOC(',
            r'\bcalloc\s*\(': 'UNIFIED_CALLOC(',
            r'\brealloc\s*\(': 'UNIFIED_REALLOC(',
            r'\bfree\s*\(': 'UNIFIED_FREE(',
            r'\bstrdup\s*\(': 'UNIFIED_STRDUP(',
            
            # memory_utils.h functions
            r'\bmemory_utils_malloc_zeroed\s*\(': 'UNIFIED_ALLOC_TRACKED(',
            r'\bmemory_utils_calloc\s*\(': 'UNIFIED_CALLOC_TRACKED(',
            r'\bmemory_utils_realloc\s*\(': 'UNIFIED_REALLOC_TRACKED(',
            r'\bmemory_utils_free\s*\(': 'UNIFIED_FREE(',
            r'\bmemory_utils_strdup\s*\(': 'UNIFIED_STRDUP(',
            
            # unified_memory_allocator functions
            r'\bunified_memory_alloc\s*\(': 'UNIFIED_ALLOC(',
            r'\bunified_memory_free\s*\(': 'UNIFIED_FREE(',
            r'\bunified_memory_realloc\s*\(': 'UNIFIED_REALLOC(',
            
            # Legacy functions
            r'\bmemory_alloc\s*\(': 'UNIFIED_ALLOC(',
            r'\bmemory_free\s*\(': 'UNIFIED_FREE(',
            r'\bmemory_realloc\s*\(': 'UNIFIED_REALLOC(',
        }
        
        self.include_pattern = r'#include\s*[<"]([^>"]+memory[^>"]*)[>"]'
        self.unified_include = '#include "core/unified_memory.h"'
        
    def should_migrate_file(self, file_path):
        """Check if file should be migrated"""
        if file_path.suffix not in ['.c', '.h']:
            return False
            
        # Skip the unified memory files themselves
        if 'unified_memory' in file_path.name:
            return False
            
        # Skip test files
        if 'test' in file_path.name.lower():
            return False
            
        return True
    
    def migrate_file(self, file_path):
        """Migrate a single file"""
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
        except Exception as e:
            print(f"Error reading {file_path}: {e}")
            return False
            
        original_content = content
        
        # Add unified memory include if not present
        if 'unified_memory.h' not in content:
            # Find the last include statement
            includes = re.findall(self.include_pattern, content)
            if includes:
                # Add after the last include
                last_include_pos = content.rfind('#include')
                if last_include_pos != -1:
                    # Find end of line
                    end_of_line = content.find('\n', last_include_pos)
                    if end_of_line != -1:
                        content = content[:end_of_line] + '\n' + self.unified_include + content[end_of_line:]
            else:
                # Add at the beginning after any comments
                lines = content.split('\n')
                insert_pos = 0
                for i, line in enumerate(lines):
                    if line.strip() and not line.strip().startswith('//') and not line.strip().startswith('/*'):
                        insert_pos = i
                        break
                lines.insert(insert_pos, self.unified_include)
                content = '\n'.join(lines)
        
        # Apply migrations
        for pattern, replacement in self.migrations.items():
            content = re.sub(pattern, replacement, content)
        
        # Only write if changes were made
        if content != original_content:
            try:
                with open(file_path, 'w', encoding='utf-8') as f:
                    f.write(content)
                return True
            except Exception as e:
                print(f"Error writing {file_path}: {e}")
                return False
                
        return False
    
    def migrate_directory(self):
        """Migrate all files in the directory"""
        migrated_files = []
        skipped_files = []
        error_files = []
        
        # Find all C/C++ files
        for file_path in self.root_dir.rglob('*'):
            if file_path.is_file() and self.should_migrate_file(file_path):
                try:
                    if self.migrate_file(file_path):
                        migrated_files.append(file_path)
                        print(f"Migrated: {file_path}")
                    else:
                        skipped_files.append(file_path)
                except Exception as e:
                    error_files.append((file_path, str(e)))
                    print(f"Error migrating {file_path}: {e}")
        
        # Print summary
        print(f"\n=== Migration Summary ===")
        print(f"Migrated files: {len(migrated_files)}")
        print(f"Skipped files: {len(skipped_files)}")
        print(f"Error files: {len(error_files)}")
        
        if error_files:
            print("\nErrors:")
            for file_path, error in error_files:
                print(f"  {file_path}: {error}")
        
        return len(migrated_files)
    
    def generate_migration_report(self):
        """Generate a detailed migration report"""
        report = []
        report.append("# Memory Management Migration Report")
        report.append(f"Generated on: {self._get_timestamp()}")
        report.append(f"Root directory: {self.root_dir}")
        report.append("")
        
        # Count files by type
        c_files = list(self.root_dir.rglob('*.c'))
        h_files = list(self.root_dir.rglob('*.h'))
        
        report.append(f"Total C files: {len(c_files)}")
        report.append(f"Total H files: {len(h_files)}")
        report.append("")
        
        # Analyze memory function usage
        total_malloc_usage = 0
        total_free_usage = 0
        total_other_usage = 0
        
        for file_path in self.root_dir.rglob('*.c'):
            try:
                with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                    content = f.read()
                    
                total_malloc_usage += len(re.findall(r'\b(malloc|calloc|realloc)\s*\(', content))
                total_free_usage += len(re.findall(r'\bfree\s*\(', content))
                total_other_usage += len(re.findall(r'\b(strdup|memory_utils_|unified_memory_)\s*\(', content))
            except:
                continue
        
        report.append("## Memory Function Usage Analysis")
        report.append(f"malloc/calloc/realloc usage: {total_malloc_usage}")
        report.append(f"free usage: {total_free_usage}")
        report.append(f"Other memory functions: {total_other_usage}")
        report.append("")
        
        # Migration recommendations
        report.append("## Migration Recommendations")
        report.append("1. Run the migration script to update all function calls")
        report.append("2. Review changes for correctness")
        report.append("3. Update build system to include unified_memory.h")
        report.append("4. Test compilation and functionality")
        report.append("5. Enable memory tracking in debug builds")
        report.append("")
        
        return '\n'.join(report)
    
    def _get_timestamp(self):
        """Get current timestamp"""
        from datetime import datetime
        return datetime.now().strftime("%Y-%m-%d %H:%M:%S")

def main():
    if len(sys.argv) != 2:
        print("Usage: python memory_migration.py <root_directory>")
        sys.exit(1)
    
    root_dir = sys.argv[1]
    if not os.path.exists(root_dir):
        print(f"Error: Directory {root_dir} does not exist")
        sys.exit(1)
    
    migrator = MemoryMigrator(root_dir)
    
    # Generate report first
    report = migrator.generate_migration_report()
    report_path = Path(root_dir) / "memory_migration_report.md"
    with open(report_path, 'w') as f:
        f.write(report)
    print(f"Migration report generated: {report_path}")
    
    # Ask for confirmation
    response = input("\nDo you want to proceed with migration? (y/N): ")
    if response.lower() != 'y':
        print("Migration cancelled")
        return
    
    # Perform migration
    migrated_count = migrator.migrate_directory()
    
    if migrated_count > 0:
        print(f"\nSuccessfully migrated {migrated_count} files")
        print("Please review the changes and test compilation")
    else:
        print("\nNo files required migration")

if __name__ == "__main__":
    main()
