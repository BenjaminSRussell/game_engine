#!/usr/bin/env python3
"""
Particle System Consolidation Script
Identifies and consolidates duplicate particle implementations
"""

import os
import re
import hashlib
from pathlib import Path
from collections import defaultdict

class ParticleConsolidator:
    def __init__(self, root_dir):
        self.root_dir = Path(root_dir)
        self.particle_files = []
        self.duplicates = {}
        self.canonical_files = {}
        
    def find_particle_files(self):
        """Find all particle-related files"""
        patterns = [
            "*particle*",
            "*Particle*", 
            "*PARTICLE*",
            "*weather*",
            "*Weather*",
            "*WEATHER*",
            "*vfx*",
            "*VFX*",
            "*effects*",
            "*Effects*"
        ]
        
        for pattern in patterns:
            for file_path in self.root_dir.rglob(pattern):
                if file_path.is_file() and file_path.suffix in ['.c', '.h', '.cpp', '.hpp']:
                    self.particle_files.append(file_path)
        
        return self.particle_files
    
    def calculate_file_hash(self, file_path):
        """Calculate hash of file content for similarity detection"""
        try:
            with open(file_path, 'rb') as f:
                content = f.read()
                return hashlib.md5(content).hexdigest()
        except Exception as e:
            print(f"Error hashing {file_path}: {e}")
            return None
    
    def analyze_file_similarity(self):
        """Analyze files for functional similarity"""
        function_signatures = defaultdict(list)
        
        for file_path in self.particle_files:
            try:
                with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                    content = f.read()
                
                # Extract function signatures
                functions = re.findall(r'\b\w+\s+\*?\s*(\w+)\s*\([^)]*\)\s*{', content)
                structs = re.findall(r'typedef\s+struct\s+\w*\s*{([^}]*)}\s*(\w+);', content, re.DOTALL)
                enums = re.findall(r'typedef\s+enum\s+\w*\s*{([^}]*)}\s*(\w+);', content, re.DOTALL)
                
                # Create signature
                signature = {
                    'functions': sorted(functions),
                    'structs': sorted([s[1] for s in structs]),
                    'enums': sorted([e[1] for e in enums]),
                    'file_path': file_path
                }
                
                # Create hash of signature
                sig_str = str(sorted(signature['functions'])) + str(sorted(signature['structs'])) + str(sorted(signature['enums']))
                sig_hash = hashlib.md5(sig_str.encode()).hexdigest()
                
                function_signatures[sig_hash].append(signature)
                
            except Exception as e:
                print(f"Error analyzing {file_path}: {e}")
        
        # Find duplicates
        for sig_hash, files in function_signatures.items():
            if len(files) > 1:
                self.duplicates[sig_hash] = files
        
        return self.duplicates
    
    def identify_canonical_files(self):
        """Identify which files to keep as canonical implementations"""
        for sig_hash, files in self.duplicates.items():
            # Prefer files with 'unified' in the name
            unified_files = [f for f in files if 'unified' in str(f['file_path']).lower()]
            if unified_files:
                self.canonical_files[sig_hash] = unified_files[0]
                continue
            
            # Prefer files in core/engine directories
            core_files = [f for f in files if 'core' in str(f['file_path']).lower() or 'engine' in str(f['file_path']).lower()]
            if core_files:
                self.canonical_files[sig_hash] = core_files[0]
                continue
            
            # Prefer header files over implementation files
            header_files = [f for f in files if f['file_path'].suffix in ['.h', '.hpp']]
            if header_files:
                self.canonical_files[sig_hash] = header_files[0]
                continue
            
            # Default to first file
            self.canonical_files[sig_hash] = files[0]
        
        return self.canonical_files
    
    def generate_consolidation_plan(self):
        """Generate a plan for consolidating duplicate files"""
        plan = {
            'keep': [],
            'remove': [],
            'backup': [],
            'migrate': []
        }
        
        for sig_hash, files in self.duplicates.items():
            canonical = self.canonical_files.get(sig_hash)
            if not canonical:
                continue
            
            for file_info in files:
                file_path = file_info['file_path']
                
                if file_path == canonical['file_path']:
                    plan['keep'].append(file_path)
                else:
                    # Check if file is a backup (has .bak extension)
                    if any(ext in str(file_path).lower() for ext in ['.bak', '.backup', '.old', '.orig']):
                        plan['backup'].append(file_path)
                    else:
                        plan['remove'].append(file_path)
                        plan['migrate'].append({
                            'from': file_path,
                            'to': canonical['file_path'],
                            'functions': file_info['functions']
                        })
        
        return plan
    
    def create_migration_script(self, plan):
        """Create a script to migrate from duplicate files to canonical ones"""
        script_lines = [
            "#!/bin/bash",
            "# Particle System Migration Script",
            "# Generated automatically - review before running",
            "",
            "set -e  # Exit on any error",
            "",
            "echo 'Starting particle system consolidation...'",
            ""
        ]
        
        # Backup files
        if plan['backup']:
            script_lines.append("# Backup duplicate files")
            for file_path in plan['backup']:
                script_lines.append(f"echo 'Backing up: {file_path}'")
                script_lines.append(f"mv '{file_path}' '{file_path}.consolidated.bak'")
            script_lines.append("")
        
        # Remove duplicates (after backup)
        if plan['remove']:
            script_lines.append("# Remove duplicate implementations")
            for file_path in plan['remove']:
                script_lines.append(f"echo 'Removing: {file_path}'")
                script_lines.append(f"rm -f '{file_path}'")
            script_lines.append("")
        
        # Create migration notes
        script_lines.extend([
            "# Create migration notes",
            "cat > PARTICLE_MIGRATION_NOTES.md << 'EOF'",
            "# Particle System Consolidation",
            "",
            "## Files Removed:",
            ""
        ])
        
        for file_path in plan['remove']:
            script_lines.append(f"- {file_path}")
        
        script_lines.extend([
            "",
            "## Canonical Files:",
            ""
        ])
        
        for file_path in plan['keep']:
            script_lines.append(f"- {file_path}")
        
        script_lines.extend([
            "",
            "## Migration Required:",
            "Update includes in source files to use unified_particle_system.h",
            "Update function calls to use unified API",
            "",
            "EOF",
            "",
            "echo 'Particle system consolidation complete!'",
            "echo 'Please review PARTICLE_MIGRATION_NOTES.md for required code updates'",
            ""
        ])
        
        return '\n'.join(script_lines)
    
    def generate_report(self):
        """Generate a detailed consolidation report"""
        report = []
        report.append("# Particle System Consolidation Report")
        report.append(f"Generated on: {self._get_timestamp()}")
        report.append(f"Root directory: {self.root_dir}")
        report.append("")
        
        # Summary statistics
        report.append("## Summary Statistics")
        report.append(f"Total particle-related files found: {len(self.particle_files)}")
        report.append(f"Duplicate groups found: {len(self.duplicates)}")
        report.append(f"Files to keep: {len(self.canonical_files)}")
        
        total_duplicates = sum(len(files) - 1 for files in self.duplicates.values())
        report.append(f"Duplicate files to remove: {total_duplicates}")
        report.append("")
        
        # File breakdown by type
        c_files = [f for f in self.particle_files if f.suffix in ['.c', '.cpp']]
        h_files = [f for f in self.particle_files if f.suffix in ['.h', '.hpp']]
        
        report.append("## File Types")
        report.append(f"C/C++ implementation files: {len(c_files)}")
        report.append(f"Header files: {len(h_files)}")
        report.append("")
        
        # Duplicate groups
        report.append("## Duplicate Analysis")
        for sig_hash, files in self.duplicates.items():
            canonical = self.canonical_files.get(sig_hash)
            report.append(f"### Group {sig_hash[:8]}...")
            report.append(f"Canonical: {canonical['file_path'] if canonical else 'None'}")
            report.append("Files in group:")
            for file_info in files:
                marker = " (CANONICAL)" if file_info['file_path'] == canonical['file_path'] else " (REMOVE)"
                report.append(f"  - {file_info['file_path']}{marker}")
            report.append("")
        
        # Recommendations
        report.append("## Recommendations")
        report.append("1. Run the generated migration script to remove duplicates")
        report.append("2. Update all includes to use unified_particle_system.h")
        report.append("3. Migrate function calls to the unified API")
        report.append("4. Test compilation and functionality")
        report.append("5. Remove old particle system dependencies")
        report.append("")
        
        return '\n'.join(report)
    
    def _get_timestamp(self):
        """Get current timestamp"""
        from datetime import datetime
        return datetime.now().strftime("%Y-%m-%d %H:%M:%S")

def main():
    if len(sys.argv) != 2:
        print("Usage: python particle_consolidation.py <root_directory>")
        sys.exit(1)
    
    root_dir = sys.argv[1]
    if not os.path.exists(root_dir):
        print(f"Error: Directory {root_dir} does not exist")
        sys.exit(1)
    
    consolidator = ParticleConsolidator(root_dir)
    
    print("Finding particle files...")
    particle_files = consolidator.find_particle_files()
    print(f"Found {len(particle_files)} particle-related files")
    
    print("Analyzing file similarities...")
    duplicates = consolidator.analyze_file_similarity()
    print(f"Found {len(duplicates)} duplicate groups")
    
    print("Identifying canonical files...")
    canonical = consolidator.identify_canonical_files()
    
    print("Generating consolidation plan...")
    plan = consolidator.generate_consolidation_plan()
    
    # Generate report
    report = consolidator.generate_report()
    report_path = Path(root_dir) / "particle_consolidation_report.md"
    with open(report_path, 'w') as f:
        f.write(report)
    print(f"Consolidation report generated: {report_path}")
    
    # Generate migration script
    migration_script = consolidator.create_migration_script(plan)
    script_path = Path(root_dir) / "particle_consolidation.sh"
    with open(script_path, 'w') as f:
        f.write(migration_script)
    os.chmod(script_path, 0o755)  # Make executable
    print(f"Migration script generated: {script_path}")
    
    # Print summary
    print(f"\n=== Consolidation Summary ===")
    print(f"Files to keep: {len(plan['keep'])}")
    print(f"Files to remove: {len(plan['remove'])}")
    print(f"Backup files: {len(plan['backup'])}")
    print(f"Migration required: {len(plan['migrate'])}")
    
    if plan['remove']:
        print(f"\nFiles to be removed:")
        for file_path in plan['remove'][:10]:  # Show first 10
            print(f"  - {file_path}")
        if len(plan['remove']) > 10:
            print(f"  ... and {len(plan['remove']) - 10} more")
    
    response = input("\nDo you want to run the consolidation script? (y/N): ")
    if response.lower() == 'y':
        print("Running consolidation script...")
        os.system(f"cd '{root_dir}' && ./particle_consolidation.sh")
    else:
        print("Consolidation cancelled. Run the script manually when ready.")

if __name__ == "__main__":
    import sys
    main()
