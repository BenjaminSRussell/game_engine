#!/usr/bin/env python3
"""
Modpack Asset Extractor

Recursively extracts assets from Minecraft modpack JAR files.
Handles nested JARs and organizes extracted content.

Usage:
    python extract_modpack_assets.py [--input downloads/modpacks] [--output extracted/raw] [--overwrite]
"""

import argparse
import json
import os
import shutil
import sys
import zipfile
from pathlib import Path
from typing import Dict, List, Set
import hashlib

SCRIPT_DIR = Path(__file__).parent
ASSETS_DIR = SCRIPT_DIR
DOWNLOADS_DIR = ASSETS_DIR / "downloads"
MODPACKS_DIR = DOWNLOADS_DIR / "modpacks"
STANDALONE_DIR = DOWNLOADS_DIR / "standalone_mods"
EXTRACTED_DIR = ASSETS_DIR / "extracted"
RAW_EXTRACT_DIR = EXTRACTED_DIR / "raw"
MANIFEST_FILE = EXTRACTED_DIR / "extraction_manifest.json"

class AssetExtractor:
    def __init__(self, input_dir: Path, output_dir: Path, overwrite: bool = False):
        self.input_dir = Path(input_dir)
        self.output_dir = Path(output_dir)
        self.overwrite = overwrite
        self.manifest = {
            'extractions': [],
            'statistics': {
                'total_jars': 0,
                'successful_extractions': 0,
                'failed_extractions': 0,
                'total_assets_found': 0,
                'asset_types': {}
            }
        }
        self.processed_jars: Set[str] = set()  # Track processed JARs by hash to avoid duplicates
    
    def calculate_file_hash(self, file_path: Path) -> str:
        """Calculate MD5 hash of a file."""
        hash_md5 = hashlib.md5()
        with open(file_path, "rb") as f:
            for chunk in iter(lambda: f.read(4096), b""):
                hash_md5.update(chunk)
        return hash_md5.hexdigest()
    
    def extract_jar(self, jar_path: Path, extract_to: Path, depth: int = 0) -> Dict:
        """
        Extract a JAR file and recursively handle nested JARs.
        
        Args:
            jar_path: Path to JAR file
            extract_to: Directory to extract to
            depth: Recursion depth (for nested JARs)
            
        Returns:
            Dictionary with extraction metadata
        """
        result = {
            'source_jar': str(jar_path),
            'extract_path': str(extract_to),
            'depth': depth,
            'assets_found': [],
            'nested_jars': [],
            'success': False,
            'error': None
        }
        
        try:
            # Calculate hash to avoid processing duplicate JARs
            jar_hash = self.calculate_file_hash(jar_path)
            if jar_hash in self.processed_jars:
                result['success'] = True
                result['note'] = 'Duplicate JAR (skipped)'
                return result
            
            self.processed_jars.add(jar_hash)
            
            extract_to.mkdir(parents=True, exist_ok=True)
            
            with zipfile.ZipFile(jar_path, 'r') as zip_ref:
                # Extract all files
                zip_ref.extractall(extract_to)
                
                # Look for assets directory
                assets_dir = extract_to / "assets"
                if assets_dir.exists():
                    result['assets_found'].append(str(assets_dir))
                
                # Look for nested JARs (mod dependencies)
                for member in zip_ref.namelist():
                    if member.endswith('.jar') and not member.startswith('META-INF/'):
                        nested_jar_path = extract_to / member
                        if nested_jar_path.exists():
                            nested_extract = extract_to / f"nested_{Path(member).stem}"
                            nested_result = self.extract_jar(nested_jar_path, nested_extract, depth + 1)
                            result['nested_jars'].append(nested_result)
                
                result['success'] = True
                result['file_count'] = len(zip_ref.namelist())
        
        except zipfile.BadZipFile:
            result['error'] = 'Not a valid ZIP/JAR file'
        except Exception as e:
            result['error'] = str(e)
        
        return result
    
    def find_jar_files(self, directory: Path) -> List[Path]:
        """Recursively find all JAR files in directory."""
        jar_files = []
        for path in directory.rglob('*.jar'):
            # Skip nested extracted JARs to avoid re-processing
            if 'nested_' not in str(path):
                jar_files.append(path)
        return jar_files
    
    def extract_from_directory(self, directory: Path) -> None:
        """
        Extract assets from all JAR files in a directory.
        
        Args:
            directory: Directory containing JAR files or modpack archives
        """
        print(f"\n🔍 Scanning directory: {directory}")
        
        jar_files = self.find_jar_files(directory)
        
        # Also look for ZIP files (modpack archives)
        zip_files = list(directory.glob('*.zip'))
        
        print(f"  Found {len(jar_files)} JAR file(s) and {len(zip_files)} ZIP file(s)")
        
        # Extract ZIP files first (modpack archives)
        for zip_file in zip_files:
            print(f"\n📦 Extracting modpack archive: {zip_file.name}")
            temp_extract = self.output_dir / f"temp_{zip_file.stem}"
            temp_extract.mkdir(parents=True, exist_ok=True)
            
            try:
                with zipfile.ZipFile(zip_file, 'r') as zip_ref:
                    zip_ref.extractall(temp_extract)
                
                # Now scan for JARs in extracted modpack
                modpack_jars = self.find_jar_files(temp_extract)
                print(f"  Found {len(modpack_jars)} mod JAR(s) in modpack")
                
                for jar in modpack_jars:
                    mod_name = jar.parent.name if jar.parent != temp_extract else jar.stem
                    extract_dest = self.output_dir / zip_file.stem / mod_name
                    
                    if extract_dest.exists() and not self.overwrite:
                        print(f"  ⏭ Skipping (already extracted): {jar.name}")
                        continue
                    
                    print(f"  📥 Extracting: {jar.name}")
                    result = self.extract_jar(jar, extract_dest)
                    self.manifest['extractions'].append(result)
                    
                    if result['success']:
                        self.manifest['statistics']['successful_extractions'] += 1
                        if result.get('assets_found'):
                            self.manifest['statistics']['total_assets_found'] += len(result['assets_found'])
                    else:
                        self.manifest['statistics']['failed_extractions'] += 1
                        print(f"    ✗ Failed: {result.get('error', 'Unknown error')}")
                
                # Cleanup temp directory
                shutil.rmtree(temp_extract)
            
            except Exception as e:
                print(f"  ✗ Failed to extract modpack archive: {e}")
        
        # Extract standalone JAR files
        for jar_file in jar_files:
            extract_dest = self.output_dir / jar_file.stem
            
            if extract_dest.exists() and not self.overwrite:
                print(f"⏭ Skipping (already extracted): {jar_file.name}")
                continue
            
            print(f"\n📥 Extracting JAR: {jar_file.name}")
            result = self.extract_jar(jar_file, extract_dest)
            self.manifest['extractions'].append(result)
            self.manifest['statistics']['total_jars'] += 1
            
            if result['success']:
                self.manifest['statistics']['successful_extractions'] += 1
                if result.get('assets_found'):
                    self.manifest['statistics']['total_assets_found'] += len(result['assets_found'])
                    print(f"  ✓ Found assets directory")
            else:
                self.manifest['statistics']['failed_extractions'] += 1
                print(f"  ✗ Failed: {result.get('error', 'Unknown error')}")
    
    def analyze_asset_types(self) -> None:
        """Analyze and count asset types in extracted directories."""
        asset_extensions = {
            '.png': 'textures',
            '.jpg': 'textures',
            '.jpeg': 'textures',
            '.tga': 'textures',
            '.json': 'models',
            '.obj': 'models',
            '.fbx': 'models',
            '.gltf': 'models',
            '.glb': 'models',
            '.ogg': 'sounds',
            '.wav': 'sounds',
            '.mp3': 'sounds',
            '.mcmeta': 'metadata',
        }
        
        stats = self.manifest['statistics']['asset_types']
        
        # Track mod-specific asset counts
        mod_asset_counts = {}
        
        for extraction in self.manifest['extractions']:
            if not extraction.get('success'):
                continue
            
            extract_path = Path(extraction['extract_path'])
            assets_dir = extract_path / "assets"
            
            if not assets_dir.exists():
                continue
            
            # Extract mod name from path
            mod_name = extract_path.name
            mod_asset_counts[mod_name] = {}
            
            for file_path in assets_dir.rglob('*'):
                if file_path.is_file():
                    ext = file_path.suffix.lower()
                    asset_type = asset_extensions.get(ext, 'unknown')
                    stats[asset_type] = stats.get(asset_type, 0) + 1
                    mod_asset_counts[mod_name][asset_type] = mod_asset_counts[mod_name].get(asset_type, 0) + 1
        
        # Add mod-specific counts to manifest
        self.manifest['mod_asset_counts'] = mod_asset_counts
    
    def save_manifest(self) -> None:
        """Save extraction manifest to JSON file."""
        MANIFEST_FILE.parent.mkdir(parents=True, exist_ok=True)
        with open(MANIFEST_FILE, 'w') as f:
            json.dump(self.manifest, f, indent=2)
        print(f"\n✓ Saved extraction manifest: {MANIFEST_FILE}")
    
    def print_summary(self) -> None:
        """Print extraction summary statistics."""
        stats = self.manifest['statistics']
        
        print("\n" + "="*60)
        print("Extraction Summary")
        print("="*60)
        print(f"Total JARs processed: {stats['total_jars']}")
        print(f"Successful extractions: {stats['successful_extractions']}")
        print(f"Failed extractions: {stats['failed_extractions']}")
        print(f"Total assets directories found: {stats['total_assets_found']}")
        
        if stats.get('asset_types'):
            print("\nAsset types found:")
            for asset_type, count in sorted(stats['asset_types'].items()):
                print(f"  {asset_type}: {count}")
        
        # Print top mods by asset count
        if self.manifest.get('mod_asset_counts'):
            print("\nTop mods by asset count:")
            mod_totals = {}
            for mod_name, asset_counts in self.manifest['mod_asset_counts'].items():
                mod_totals[mod_name] = sum(asset_counts.values())
            
            for mod_name, total in sorted(mod_totals.items(), key=lambda x: x[1], reverse=True)[:10]:
                print(f"  {mod_name}: {total} assets")

def main():
    parser = argparse.ArgumentParser(description='Extract assets from Minecraft modpack JAR files')
    parser.add_argument('--input', type=str, default=str(DOWNLOADS_DIR),
                       help='Input directory containing modpacks/JARs')
    parser.add_argument('--output', type=str, default=str(RAW_EXTRACT_DIR),
                       help='Output directory for extracted assets')
    parser.add_argument('--overwrite', action='store_true',
                       help='Overwrite existing extractions')
    parser.add_argument('--modpacks-only', action='store_true',
                       help='Process only modpack archives')
    parser.add_argument('--standalone-only', action='store_true',
                       help='Process only standalone mod JARs')
    
    args = parser.parse_args()
    
    input_dir = Path(args.input)
    output_dir = Path(args.output)
    
    if not input_dir.exists():
        print(f"Error: Input directory does not exist: {input_dir}")
        sys.exit(1)
    
    output_dir.mkdir(parents=True, exist_ok=True)
    
    extractor = AssetExtractor(input_dir, output_dir, args.overwrite)
    
    print("="*60)
    print("Modpack Asset Extractor")
    print("="*60)
    print(f"Input directory: {input_dir}")
    print(f"Output directory: {output_dir}")
    print(f"Overwrite existing: {args.overwrite}")
    
    if args.modpacks_only:
        extractor.extract_from_directory(MODPACKS_DIR)
    elif args.standalone_only:
        extractor.extract_from_directory(STANDALONE_DIR)
    else:
        extractor.extract_from_directory(input_dir)
    
    extractor.analyze_asset_types()
    extractor.save_manifest()
    extractor.print_summary()
    
    print("\n" + "="*60)
    print("Extraction Complete!")
    print("="*60)
    print(f"\nNext steps:")
    print(f"1. Review extracted assets in: {output_dir}")
    print(f"2. Organize assets: python organize_extracted_assets.py")

if __name__ == '__main__':
    main()

