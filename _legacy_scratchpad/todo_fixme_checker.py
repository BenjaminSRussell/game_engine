#!/usr/bin/env python3
"""
TODO/FIXME Checker for Minecraft v2 Engine
Part of TODO-0060: No TODOs or FIXMEs in main code paths

This script scans source files to find TODO and FIXME comments
and helps eliminate them from main code paths.
"""

import os
import re
import sys
import argparse
from pathlib import Path
from typing import List, Dict, Tuple, Set
from datetime import datetime

class TodoFixmeChecker:
    def __init__(self, root_dir: str):
        self.root_dir = Path(root_dir)
        self.todo_items = []
        self.fixme_items = []
        self.excluded_dirs = {
            '.git', '.vscode', '.claude', 'build', 'cmake-build-debug',
            'node_modules', '__pycache__', '.pytest_cache', 'Vendor'
        }
        self.excluded_files = {
            'test_', 'mock_', 'stub_', 'example_', 'demo_'
        }
        
    def is_source_file(self, file_path: Path) -> bool:
        """Check if file is a source code file."""
        source_extensions = {'.c', '.cpp', '.cc', '.cxx', '.h', '.hpp', '.hxx', '.m', '.mm', '.py', '.swift'}
        return file_path.suffix.lower() in source_extensions
    
    def should_exclude_file(self, file_path: Path) -> bool:
        """Check if file should be excluded from analysis."""
        # Check if any part of the path is in excluded directories
        for part in file_path.parts:
            if part in self.excluded_dirs:
                return True
        
        # Check if filename starts with any excluded prefix
        filename = file_path.name.lower()
        for prefix in self.excluded_files:
            if filename.startswith(prefix):
                return True
        
        return False
    
    def is_main_code_path(self, file_path: Path) -> bool:
        """Determine if file is in main code path vs test/example code."""
        path_parts = [part.lower() for part in file_path.parts]
        
        # Exclude test, example, demo directories
        exclude_patterns = ['test', 'tests', 'example', 'examples', 'demo', 'demos', 'sample', 'samples']
        for pattern in exclude_patterns:
            if pattern in path_parts:
                return False
        
        # Include main source directories
        include_patterns = ['src', 'source', 'lib', 'libs', 'engine', 'core', 'systems']
        for pattern in include_patterns:
            if pattern in path_parts:
                return True
        
        # If in root and is a source file, consider it main code
        if len(path_parts) == 1:
            return True
        
        return True
    
    def scan_file(self, file_path: Path) -> None:
        """Scan a single file for TODO and FIXME comments."""
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                lines = f.readlines()
            
            is_main_path = self.is_main_code_path(file_path)
            
            for line_num, line in enumerate(lines, 1):
                line_stripped = line.strip()
                
                # Check for TODO comments
                todo_match = re.search(r'(?i)//\s*TODO\b|/\*\s*TODO\b|#\s*TODO\b', line_stripped)
                if todo_match:
                    todo_text = line_stripped[todo_match.start():].strip()
                    self.todo_items.append({
                        'file': file_path,
                        'line': line_num,
                        'text': todo_text,
                        'is_main_path': is_main_path,
                        'severity': 'high' if is_main_path else 'medium'
                    })
                
                # Check for FIXME comments
                fixme_match = re.search(r'(?i)//\s*FIXME\b|/\*\s*FIXME\b|#\s*FIXME\b', line_stripped)
                if fixme_match:
                    fixme_text = line_stripped[fixme_match.start():].strip()
                    self.fixme_items.append({
                        'file': file_path,
                        'line': line_num,
                        'text': fixme_text,
                        'is_main_path': is_main_path,
                        'severity': 'high' if is_main_path else 'medium'
                    })
        
        except Exception as e:
            print(f"Error scanning {file_path}: {e}")
    
    def scan_directory(self) -> None:
        """Scan all source files in the directory tree."""
        print(f"Scanning directory: {self.root_dir}")
        
        for file_path in self.root_dir.rglob('*'):
            if file_path.is_file() and self.is_source_file(file_path) and not self.should_exclude_file(file_path):
                self.scan_file(file_path)
    
    def categorize_issues(self) -> Dict:
        """Categorize TODO/FIXME items by severity and type."""
        main_path_todos = [item for item in self.todo_items if item['is_main_path']]
        main_path_fixmes = [item for item in self.fixme_items if item['is_main_path']]
        test_path_todos = [item for item in self.todo_items if not item['is_main_path']]
        test_path_fixmes = [item for item in self.fixme_items if not item['is_main_path']]
        
        return {
            'main_path_todos': main_path_todos,
            'main_path_fixmes': main_path_fixmes,
            'test_path_todos': test_path_todos,
            'test_path_fixmes': test_path_fixmes,
            'total_main_issues': len(main_path_todos) + len(main_path_fixmes),
            'total_test_issues': len(test_path_todos) + len(test_path_fixmes),
            'total_issues': len(self.todo_items) + len(self.fixme_items)
        }
    
    def generate_fix_suggestions(self) -> List[Dict]:
        """Generate suggestions for fixing TODO/FIXME items."""
        suggestions = []
        
        # Group by file to provide file-level suggestions
        files_with_issues = {}
        
        for item in self.todo_items + self.fixme_items:
            file_path = str(item['file'])
            if file_path not in files_with_issues:
                files_with_issues[file_path] = []
            files_with_issues[file_path].append(item)
        
        for file_path, issues in files_with_issues.items():
            if any(issue['is_main_path'] for issue in issues):
                suggestions.append({
                    'file': file_path,
                    'main_path_issues': [issue for issue in issues if issue['is_main_path']],
                    'test_path_issues': [issue for issue in issues if not issue['is_main_path']],
                    'priority': 'high' if any(issue['is_main_path'] for issue in issues) else 'medium'
                })
        
        return suggestions
    
    def print_report(self) -> None:
        """Print a detailed TODO/FIXME report."""
        categories = self.categorize_issues()
        
        print("\n" + "="*60)
        print("TODO/FIXME ANALYSIS REPORT")
        print("TODO-0060: No TODOs or FIXMEs in main code paths")
        print("="*60)
        
        print(f"\nSUMMARY:")
        print(f"  Total TODOs found: {len(self.todo_items)}")
        print(f"  Total FIXMEs found: {len(self.fixme_items)}")
        print(f"  Main code path issues: {categories['total_main_issues']}")
        print(f"    - TODOs in main paths: {len(categories['main_path_todos'])}")
        print(f"    - FIXMEs in main paths: {len(categories['main_path_fixmes'])}")
        print(f"  Test/example issues: {categories['total_test_issues']}")
        print(f"    - TODOs in test paths: {len(categories['test_path_todos'])}")
        print(f"    - FIXMEs in test paths: {len(categories['test_path_fixmes'])}")
        
        # Main code path issues (critical)
        if categories['main_path_todos']:
            print(f"\n🚨 CRITICAL: TODOs in Main Code Paths ({len(categories['main_path_todos'])}):")
            for item in categories['main_path_todos'][:15]:  # Show first 15
                rel_path = item['file'].relative_to(self.root_dir)
                print(f"  {rel_path}:{item['line']} - {item['text']}")
            
            if len(categories['main_path_todos']) > 15:
                print(f"  ... and {len(categories['main_path_todos']) - 15} more")
        
        if categories['main_path_fixmes']:
            print(f"\n🚨 CRITICAL: FIXMEs in Main Code Paths ({len(categories['main_path_fixmes'])}):")
            for item in categories['main_path_fixmes'][:15]:  # Show first 15
                rel_path = item['file'].relative_to(self.root_dir)
                print(f"  {rel_path}:{item['line']} - {item['text']}")
            
            if len(categories['main_path_fixmes']) > 15:
                print(f"  ... and {len(categories['main_path_fixmes']) - 15} more")
        
        # Test/example issues (less critical)
        if categories['test_path_todos']:
            print(f"\n⚠️  TODOs in Test/Example Code ({len(categories['test_path_todos'])}):")
            for item in categories['test_path_todos'][:10]:  # Show first 10
                rel_path = item['file'].relative_to(self.root_dir)
                print(f"  {rel_path}:{item['line']} - {item['text']}")
            
            if len(categories['test_path_todos']) > 10:
                print(f"  ... and {len(categories['test_path_todos']) - 10} more")
        
        if categories['test_path_fixmes']:
            print(f"\n⚠️  FIXMEs in Test/Example Code ({len(categories['test_path_fixmes'])}):")
            for item in categories['test_path_fixmes'][:10]:  # Show first 10
                rel_path = item['file'].relative_to(self.root_dir)
                print(f"  {rel_path}:{item['line']} - {item['text']}")
            
            if len(categories['test_path_fixmes']) > 10:
                print(f"  ... and {len(categories['test_path_fixmes']) - 10} more")
        
        # File-level suggestions
        suggestions = self.generate_fix_suggestions()
        high_priority_files = [s for s in suggestions if s['priority'] == 'high']
        
        if high_priority_files:
            print(f"\n📋 FILES REQUIRING IMMEDIATE ATTENTION:")
            for suggestion in high_priority_files[:10]:
                print(f"  {suggestion['file']}:")
                print(f"    - {len(suggestion['main_path_issues'])} main path issues")
                if suggestion['test_path_issues']:
                    print(f"    - {len(suggestion['test_path_issues'])} test path issues")
        
        # Recommendations
        print(f"\n📝 RECOMMENDATIONS:")
        if categories['total_main_issues'] > 0:
            print(f"  1. URGENT: Address {categories['total_main_issues']} main code path issues")
            print(f"     - These block production readiness")
            print(f"     - Focus on FIXMEs first (critical bugs)")
            print(f"     - Then address TODOs (missing features)")
        
        if categories['total_test_issues'] > 0:
            print(f"  2. Address {categories['total_test_issues']} test/example issues")
            print(f"     - Lower priority but should be cleaned up")
            print(f"     - Improves code quality and maintainability")
        
        print(f"  3. Establish code review process to prevent new TODOs/FIXMEs")
        print(f"  4. Use issue tracking system instead of inline TODOs")
        print(f"  5. Consider automated pre-commit hooks to catch new issues")
        
        # Final assessment
        print(f"\n🎯 FINAL ASSESSMENT:")
        if categories['total_main_issues'] == 0:
            print(f"  ✅ TODO-0060: REQUIREMENT MET")
            print(f"  ✅ No TODOs or FIXMEs in main code paths")
            print(f"  ✅ Code is production-ready from TODO/FIXME perspective")
        else:
            print(f"  ❌ TODO-0060: REQUIREMENT NOT MET")
            print(f"  ❌ {categories['total_main_issues']} issues found in main code paths")
            print(f"  ❌ Code is NOT production-ready due to TODO/FIXME items")
        
        print("="*60)
    
    def generate_fix_script(self, output_file: str) -> None:
        """Generate a script to help fix the issues."""
        suggestions = self.generate_fix_suggestions()
        
        with open(output_file, 'w') as f:
            f.write("#!/bin/bash\n")
            f.write("#\n")
            f.write("# Auto-generated script to help fix TODO/FIXME issues\n")
            f.write("# Generated on: " + datetime.now().strftime("%Y-%m-%d %H:%M:%S") + "\n")
            f.write("#\n\n")
            
            f.write("echo \"TODO/FIXME Fix Helper Script\"\n")
            f.write("echo \"=============================\"\n\n")
            
            # Group by priority
            high_priority_files = [s for s in suggestions if s['priority'] == 'high']
            medium_priority_files = [s for s in suggestions if s['priority'] == 'medium']
            
            if high_priority_files:
                f.write("echo \"HIGH PRIORITY - Main Code Path Issues:\"\n")
                f.write("echo \"\"\n")
                for suggestion in high_priority_files:
                    f.write(f"echo \"File: {suggestion['file']}\"\n")
                    for issue in suggestion['main_path_issues']:
                        f.write(f"echo \"  Line {issue['line']}: {issue['text']}\"\n")
                    f.write("echo \"\"\n)
            
            if medium_priority_files:
                f.write("echo \"MEDIUM PRIORITY - Test/Example Issues:\"\n")
                f.write("echo \"\"\n")
                for suggestion in medium_priority_files:
                    if suggestion['test_path_issues']:
                        f.write(f"echo \"File: {suggestion['file']}\"\n")
                        for issue in suggestion['test_path_issues']:
                            f.write(f"echo \"  Line {issue['line']}: {issue['text']}\"\n")
                        f.write("echo \"\"\n")
            
            f.write("echo \"Use your editor to open these files and address the issues.\"\n")
            f.write("echo \"After fixing, run this checker again to verify.\"\n")

def main():
    parser = argparse.ArgumentParser(description='Check for TODO and FIXME comments in source code')
    parser.add_argument('directory', help='Root directory to scan')
    parser.add_argument('--verbose', '-v', action='store_true', help='Verbose output')
    parser.add_argument('--output', '-o', help='Output report to file')
    parser.add_argument('--fix-script', help='Generate fix helper script')
    
    args = parser.parse_args()
    
    if not os.path.exists(args.directory):
        print(f"Error: Directory '{args.directory}' does not exist")
        sys.exit(1)
    
    checker = TodoFixmeChecker(args.directory)
    checker.scan_directory()
    
    if args.output:
        with open(args.output, 'w') as f:
            original_stdout = sys.stdout
            sys.stdout = f
            checker.print_report()
            sys.stdout = original_stdout
        print(f"Report saved to: {args.output}")
    else:
        checker.print_report()
    
    if args.fix_script:
        checker.generate_fix_script(args.fix_script)
        print(f"Fix helper script generated: {args.fix_script}")
        print(f"Run: chmod +x {args.fix_script} && ./{args.fix_script}")
    
    # Exit with appropriate code
    categories = checker.categorize_issues()
    if categories['total_main_issues'] == 0:
        sys.exit(0)
    else:
        sys.exit(1)

if __name__ == '__main__':
    main()
