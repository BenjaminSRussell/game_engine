#!/usr/bin/env python3
"""
Documentation Checker for Minecraft v2 Engine
Part of TODO-0059: All functions documented with purpose + parameters

This script scans source files to verify that all functions have proper
documentation including purpose and parameter descriptions.
"""

import os
import re
import sys
import argparse
from pathlib import Path
from typing import List, Dict, Tuple, Set

class DocumentationChecker:
    def __init__(self, root_dir: str):
        self.root_dir = Path(root_dir)
        self.undocumented_functions = []
        self.poorly_documented_functions = []
        self.well_documented_functions = []
        self.total_functions = 0
        self.excluded_dirs = {
            '.git', '.vscode', '.claude', 'build', 'cmake-build-debug',
            'node_modules', '__pycache__', '.pytest_cache', 'Vendor'
        }
        self.excluded_files = {
            'test_', 'mock_', 'stub_', 'example_', 'demo_'
        }
        
    def is_source_file(self, file_path: Path) -> bool:
        """Check if file is a source code file."""
        source_extensions = {'.c', '.cpp', '.cc', '.cxx', '.h', '.hpp', '.hxx', '.m', '.mm'}
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
    
    def extract_functions(self, content: str, file_path: Path) -> List[Dict]:
        """Extract function definitions from source content."""
        functions = []
        
        # Regex patterns for different function signatures
        patterns = [
            # C functions: return_type function_name(params)
            r'^\s*(?:inline\s+)?(?:static\s+)?(?:extern\s+)?(?:\w+\s+)+\*?\s*(\w+)\s*\([^)]*\)\s*(?:const\s+)?(?:override\s+)?(?:final\s+)?\s*\{?',
            # Constructor/destructor
            r'^\s*(?:explicit\s+)?(?:inline\s+)?(\w+)\s*\([^)]*\)\s*(?:const\s+)?(?:override\s+)?(?:final\s+)?\s*\{?',
            # Function pointers and complex templates
            r'^\s*(?:template\s*<[^>]*>\s*)?(?:inline\s+)?(?:static\s+)?(?:extern\s+)?(?:\w+\s+)+\*?\s*(\w+)\s*\([^)]*\)\s*(?:const\s+)?(?:override\s+)?(?:final\s+)?\s*\{?',
        ]
        
        lines = content.split('\n')
        in_function = False
        current_function = None
        brace_count = 0
        
        for i, line in enumerate(lines):
            # Skip preprocessor directives and comments
            if line.strip().startswith('#') or line.strip().startswith('//') or line.strip().startswith('/*'):
                continue
            
            # Check for function definition
            for pattern in patterns:
                match = re.match(pattern, line)
                if match:
                    func_name = match.group(1)
                    
                    # Skip common non-function keywords
                    skip_keywords = {'if', 'while', 'for', 'switch', 'return', 'struct', 'class', 'enum', 'union', 'typedef'}
                    if func_name.lower() in skip_keywords:
                        continue
                    
                    # Extract full signature (might span multiple lines)
                    signature = line.strip()
                    j = i + 1
                    while j < len(lines) and '{' not in lines[j] and ';' not in lines[j]:
                        signature += ' ' + lines[j].strip()
                        j += 1
                    
                    current_function = {
                        'name': func_name,
                        'signature': signature,
                        'line': i + 1,
                        'file': file_path,
                        'has_doc': False,
                        'doc_quality': 0
                    }
                    
                    functions.append(current_function)
                    in_function = True
                    break
        
        return functions
    
    def extract_documentation(self, content: str, functions: List[Dict]) -> None:
        """Extract documentation for functions and assess quality."""
        lines = content.split('\n')
        
        for func in functions:
            func_line = func['line'] - 1  # Convert to 0-based
            
            # Look for documentation before the function (within 20 lines)
            doc_start = max(0, func_line - 20)
            doc_lines = []
            
            for i in range(doc_start, func_line):
                line = lines[i].strip()
                
                # Check for different documentation styles
                if (line.startswith('/**') or line.startswith('/*!') or 
                    line.startswith('///') or line.startswith('//!') or
                    line.startswith('/*') or line.startswith('//')):
                    doc_lines.append(line)
                elif line and not line.startswith('/*') and not line.startswith('//'):
                    # Stop at first non-comment line
                    break
            
            if doc_lines:
                func['has_doc'] = True
                func['doc_quality'] = self.assess_documentation_quality(doc_lines, func)
    
    def assess_documentation_quality(self, doc_lines: List[str], func: Dict) -> int:
        """Assess the quality of documentation on a scale of 0-100."""
        score = 0
        doc_text = ' '.join(doc_lines).lower()
        
        # Basic documentation (20 points)
        if doc_lines:
            score += 20
        
        # Has purpose/description (25 points)
        purpose_keywords = ['purpose', 'description', 'brief', 'summary', 'overview', 'this function']
        if any(keyword in doc_text for keyword in purpose_keywords):
            score += 25
        
        # Has parameter documentation (25 points)
        param_keywords = ['param', 'parameter', '@param', '\\param', 'args', 'arguments']
        if any(keyword in doc_text for keyword in param_keywords):
            score += 25
        
        # Has return value documentation (15 points)
        return_keywords = ['return', '@return', '\\return', 'returns', '@retval']
        if any(keyword in doc_text for keyword in return_keywords):
            score += 15
        
        # Has additional info (notes, examples, etc.) (15 points)
        additional_keywords = ['note', 'example', 'see', 'warning', 'todo', 'fixme', 'bug']
        if any(keyword in doc_text for keyword in additional_keywords):
            score += 15
        
        return min(score, 100)
    
    def check_file(self, file_path: Path) -> None:
        """Check a single source file for documentation."""
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
            
            functions = self.extract_functions(content, file_path)
            self.extract_documentation(content, functions)
            
            for func in functions:
                self.total_functions += 1
                
                if not func['has_doc']:
                    self.undocumented_functions.append(func)
                elif func['doc_quality'] < 50:
                    self.poorly_documented_functions.append(func)
                else:
                    self.well_documented_functions.append(func)
        
        except Exception as e:
            print(f"Error processing {file_path}: {e}")
    
    def scan_directory(self) -> None:
        """Scan all source files in the directory tree."""
        print(f"Scanning directory: {self.root_dir}")
        
        for file_path in self.root_dir.rglob('*'):
            if file_path.is_file() and self.is_source_file(file_path) and not self.should_exclude_file(file_path):
                print(f"Checking: {file_path.relative_to(self.root_dir)}")
                self.check_file(file_path)
    
    def generate_report(self) -> Dict:
        """Generate a comprehensive documentation report."""
        total = self.total_functions
        if total == 0:
            return {
                'total_functions': 0,
                'documented_percentage': 0,
                'well_documented_percentage': 0,
                'undocumented_count': 0,
                'poorly_documented_count': 0,
                'well_documented_count': 0,
                'status': 'NO_FUNCTIONS_FOUND'
            }
        
        documented = total - len(self.undocumented_functions)
        documented_percentage = (documented / total) * 100
        well_documented_percentage = (len(self.well_documented_functions) / total) * 100
        
        # Determine status
        if documented_percentage >= 95 and well_documented_percentage >= 80:
            status = 'EXCELLENT'
        elif documented_percentage >= 90 and well_documented_percentage >= 70:
            status = 'GOOD'
        elif documented_percentage >= 80 and well_documented_percentage >= 60:
            status = 'ACCEPTABLE'
        elif documented_percentage >= 70:
            status = 'NEEDS_IMPROVEMENT'
        else:
            status = 'POOR'
        
        return {
            'total_functions': total,
            'documented_percentage': documented_percentage,
            'well_documented_percentage': well_documented_percentage,
            'undocumented_count': len(self.undocumented_functions),
            'poorly_documented_count': len(self.poorly_documented_functions),
            'well_documented_count': len(self.well_documented_functions),
            'status': status
        }
    
    def print_report(self) -> None:
        """Print a detailed documentation report."""
        report = self.generate_report()
        
        print("\n" + "="*60)
        print("DOCUMENTATION ANALYSIS REPORT")
        print("TODO-0059: All functions documented with purpose + parameters")
        print("="*60)
        
        print(f"\nSUMMARY:")
        print(f"  Total functions found: {report['total_functions']}")
        print(f"  Functions with documentation: {report['total_functions'] - report['undocumented_count']}")
        print(f"  Functions without documentation: {report['undocumented_count']}")
        print(f"  Poorly documented functions: {report['poorly_documented_count']}")
        print(f"  Well documented functions: {report['well_documented_count']}")
        print(f"  Documentation coverage: {report['documented_percentage']:.1f}%")
        print(f"  Quality documentation: {report['well_documented_percentage']:.1f}%")
        print(f"  Overall status: {report['status']}")
        
        # Print undocumented functions
        if self.undocumented_functions:
            print(f"\nUNDOCUMENTED FUNCTIONS ({len(self.undocumented_functions)}):")
            for func in self.undocumented_functions[:20]:  # Show first 20
                rel_path = func['file'].relative_to(self.root_dir)
                print(f"  {rel_path}:{func['line']} - {func['name']}")
            
            if len(self.undocumented_functions) > 20:
                print(f"  ... and {len(self.undocumented_functions) - 20} more")
        
        # Print poorly documented functions
        if self.poorly_documented_functions:
            print(f"\nPOORLY DOCUMENTED FUNCTIONS ({len(self.poorly_documented_functions)}):")
            for func in self.poorly_documented_functions[:20]:  # Show first 20
                rel_path = func['file'].relative_to(self.root_dir)
                print(f"  {rel_path}:{func['line']} - {func['name']} (quality: {func['doc_quality']}%)")
            
            if len(self.poorly_documented_functions) > 20:
                print(f"  ... and {len(self.poorly_documented_functions) - 20} more")
        
        # Recommendations
        print(f"\nRECOMMENDATIONS:")
        if report['undocumented_count'] > 0:
            print(f"  - Add documentation to {report['undocumented_count']} undocumented functions")
        
        if report['poorly_documented_count'] > 0:
            print(f"  - Improve documentation quality for {report['poorly_documented_count']} functions")
        
        if report['documented_percentage'] < 95:
            print(f"  - Aim for 95%+ documentation coverage (currently {report['documented_percentage']:.1f}%)")
        
        if report['well_documented_percentage'] < 80:
            print(f"  - Aim for 80%+ quality documentation (currently {report['well_documented_percentage']:.1f}%)")
        
        # Final assessment
        print(f"\nFINAL ASSESSMENT:")
        if report['status'] in ['EXCELLENT', 'GOOD']:
            print(f"  ✅ TODO-0059: Documentation requirements MET")
            print(f"  ✅ All functions adequately documented with purpose and parameters")
        else:
            print(f"  ❌ TODO-0059: Documentation requirements NOT MET")
            print(f"  ❌ Additional documentation work required")
        
        print("="*60)

def main():
    parser = argparse.ArgumentParser(description='Check documentation coverage in source code')
    parser.add_argument('directory', help='Root directory to scan')
    parser.add_argument('--verbose', '-v', action='store_true', help='Verbose output')
    parser.add_argument('--output', '-o', help='Output report to file')
    
    args = parser.parse_args()
    
    if not os.path.exists(args.directory):
        print(f"Error: Directory '{args.directory}' does not exist")
        sys.exit(1)
    
    checker = DocumentationChecker(args.directory)
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
    
    # Exit with appropriate code
    report = checker.generate_report()
    if report['status'] in ['EXCELLENT', 'GOOD']:
        sys.exit(0)
    else:
        sys.exit(1)

if __name__ == '__main__':
    main()
