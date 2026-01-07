#!/usr/bin/env python3
"""
Asset Organization Script

Organizes extracted modpack assets into categories (vehicles, magic, environment, etc.)
based on mod names and asset paths.

Usage:
    python organize_extracted_assets.py [--input extracted/raw] [--output extracted] [--manifest manifest.json]
"""

import argparse
import json
import shutil
import sys
from pathlib import Path
from typing import Dict, List, Set
import hashlib

SCRIPT_DIR = Path(__file__).parent
ASSETS_DIR = SCRIPT_DIR
EXTRACTED_DIR = ASSETS_DIR / "extracted"
RAW_EXTRACT_DIR = EXTRACTED_DIR / "raw"
MANIFEST_FILE = EXTRACTED_DIR / "extraction_manifest.json"
ORG_MANIFEST_FILE = EXTRACTED_DIR / "organization_manifest.json"

# Mod name patterns for categorization
CATEGORY_PATTERNS = {
    'vehicles': [
        'vehicle', 'car', 'atv', 'tank', 'plane', 'aircraft', 'ship', 'boat', 'vessel',
        'train', 'railroad', 'railway', 'locomotive', 'cart', 'truck', 'bike', 'motorcycle',
        'mrcrayfish', 'flans', 'immersive_railroading', 'create', 'davinci', 'small_ships',
        'trackwork', 'ultimate_car'
    ],
    'magic': [
        'thaumcraft', 'botania', 'ars', 'nouveau', 'wizardry', 'blood_magic', 'astral',
        'sorcery', 'mahou', 'tsukai', 'eidolon', 'magic', 'spell', 'wand', 'glyph',
        'rune', 'altar', 'node', 'eldritch', 'mana', 'vis'
    ],
    'tech': [
        'immersive_engineering', 'mekanism', 'gregtech', 'applied_energistics', 'tech',
        'machine', 'factory', 'industrial', 'chemistry', 'reactor', 'generator', 'cable',
        'circuit', 'computer', 'automation'
    ],
    'mobs': [
        'ice_and_fire', 'alex', 'mob', 'mowzie', 'lycanite', 'creature', 'dragon', 'beast',
        'animal', 'boss', 'monster', 'npc'
    ],
    'dimensions': [
        'twilight', 'forest', 'aether', 'biome', 'dimension', 'realm', 'world', 'nether',
        'end', 'void', 'betweenlands'
    ],
    'environment': [
        'tree', 'plant', 'flower', 'grass', 'terrain', 'rock', 'stone', 'dirt', 'sand',
        'water', 'lava', 'cave', 'dungeon', 'structure', 'building', 'decoration'
    ]
}

class AssetOrganizer:
    def __init__(self, input_dir: Path, output_base: Path, overwrite: bool = False):
        self.input_dir = Path(input_dir)
        self.output_base = Path(output_base)
        self.overwrite = overwrite
        self.manifest = {
            'organization': [],
            'statistics': {
                'total_mods_processed': 0,
                'assets_copied': 0,
                'assets_skipped': 0,
                'categories': {}
            },
            'file_hashes': {}  # Track duplicates by hash
        }
        self.seen_hashes: Dict[str, str] = {}  # hash -> target_path
    
    def calculate_file_hash(self, file_path: Path) -> str:
        """Calculate MD5 hash of a file."""
        try:
            hash_md5 = hashlib.md5()
            with open(file_path, "rb") as f:
                for chunk in iter(lambda: f.read(4096), b""):
                    hash_md5.update(chunk)
            return hash_md5.hexdigest()
        except Exception as e:
            print(f"    ⚠ Could not hash {file_path}: {e}")
            return ""
    
    def categorize_mod(self, mod_name: str) -> List[str]:
        """
        Determine which categories a mod belongs to based on its name.
        
        Args:
            mod_name: Name of the mod
            
        Returns:
            List of category names
        """
        mod_lower = mod_name.lower().replace('-', '_').replace(' ', '_')
        categories = set()
        
        for category, patterns in CATEGORY_PATTERNS.items():
            for pattern in patterns:
                if pattern in mod_lower:
                    categories.add(category)
                    break
        
        # If no category matched, put in environment as default
        if not categories:
            categories.add('environment')
        
        return list(categories)
    
    def copy_asset_file(self, source: Path, dest: Path) -> bool:
        """
        Copy an asset file, handling duplicates.
        
        Args:
            source: Source file path
            dest: Destination file path
            
        Returns:
            True if file was copied, False if skipped
        """
        if not source.exists():
            return False
        
        # Calculate hash to detect duplicates
        file_hash = self.calculate_file_hash(source)
        if file_hash:
            if file_hash in self.seen_hashes:
                # Duplicate file, skip but record reference
                self.manifest['statistics']['assets_skipped'] += 1
                return False
            self.seen_hashes[file_hash] = str(dest)
        
        # Copy file
        dest.parent.mkdir(parents=True, exist_ok=True)
        try:
            shutil.copy2(source, dest)
            self.manifest['statistics']['assets_copied'] += 1
            return True
        except Exception as e:
            print(f"    ✗ Failed to copy {source.name}: {e}")
            return False
    
    def organize_mod_assets(self, mod_extract_dir: Path, mod_name: str) -> Dict:
        """
        Organize assets from a single mod's extraction directory.
        
        Args:
            mod_extract_dir: Directory containing extracted mod assets
            mod_name: Name of the mod
            
        Returns:
            Organization metadata
        """
        result = {
            'mod_name': mod_name,
            'source_path': str(mod_extract_dir),
            'categories': [],
            'assets_organized': 0,
            'assets_by_category': {},
            'success': False
        }
        
        assets_dir = mod_extract_dir / "assets"
        if not assets_dir.exists():
            return result
        
        # Determine categories for this mod
        categories = self.categorize_mod(mod_name)
        result['categories'] = categories
        
        # Organize assets into categories
        for category in categories:
            category_dir = self.output_base / category / mod_name
            category_dir.mkdir(parents=True, exist_ok=True)
            
            # Copy assets directory structure
            assets_copied = 0
            for asset_file in assets_dir.rglob('*'):
                if asset_file.is_file():
                    # Preserve relative path structure
                    rel_path = asset_file.relative_to(assets_dir)
                    dest_path = category_dir / rel_path
                    
                    if self.copy_asset_file(asset_file, dest_path):
                        assets_copied += 1
            
            if assets_copied > 0:
                result['assets_by_category'][category] = assets_copied
                result['assets_organized'] += assets_copied
                
                # Update statistics
                if category not in self.manifest['statistics']['categories']:
                    self.manifest['statistics']['categories'][category] = 0
                self.manifest['statistics']['categories'][category] += assets_copied
        
        result['success'] = True
        return result
    
    def organize_all(self) -> None:
        """Organize all extracted mod assets."""
        print("\n" + "="*60)
        print("Organizing Extracted Assets")
        print("="*60)
        print(f"Input directory: {self.input_dir}")
        print(f"Output base directory: {self.output_base}")
        
        # Find all mod extraction directories
        mod_dirs = []
        
        # Look for direct mod directories
        for item in self.input_dir.iterdir():
            if item.is_dir():
                # Check if it contains an assets directory
                if (item / "assets").exists():
                    mod_dirs.append(item)
                else:
                    # Might be a modpack directory with subdirectories
                    for subdir in item.iterdir():
                        if subdir.is_dir() and (subdir / "assets").exists():
                            mod_dirs.append(subdir)
        
        print(f"\nFound {len(mod_dirs)} mod directory(ies) to organize\n")
        
        for mod_dir in mod_dirs:
            mod_name = mod_dir.name
            print(f"\n📦 [{mod_name}]")
            
            result = self.organize_mod_assets(mod_dir, mod_name)
            self.manifest['organization'].append(result)
            self.manifest['statistics']['total_mods_processed'] += 1
            
            if result['success']:
                categories_str = ', '.join(result['categories'])
                print(f"  ✓ Categorized as: {categories_str}")
                print(f"  ✓ Organized {result['assets_organized']} asset(s)")
                for cat, count in result['assets_by_category'].items():
                    print(f"    - {cat}: {count} file(s)")
            else:
                print(f"  ⚠ No assets directory found")
    
    def save_manifest(self) -> None:
        """Save organization manifest to JSON file."""
        ORG_MANIFEST_FILE.parent.mkdir(parents=True, exist_ok=True)
        with open(ORG_MANIFEST_FILE, 'w') as f:
            json.dump(self.manifest, f, indent=2)
        print(f"\n✓ Saved organization manifest: {ORG_MANIFEST_FILE}")
    
    def print_summary(self) -> None:
        """Print organization summary statistics."""
        stats = self.manifest['statistics']
        
        print("\n" + "="*60)
        print("Organization Summary")
        print("="*60)
        print(f"Total mods processed: {stats['total_mods_processed']}")
        print(f"Assets copied: {stats['assets_copied']}")
        print(f"Assets skipped (duplicates): {stats['assets_skipped']}")
        
        if stats.get('categories'):
            print("\nAssets by category:")
            for category, count in sorted(stats['categories'].items()):
                print(f"  {category}: {count} file(s)")

def main():
    parser = argparse.ArgumentParser(description='Organize extracted modpack assets into categories')
    parser.add_argument('--input', type=str, default=str(RAW_EXTRACT_DIR),
                       help='Input directory containing extracted mods')
    parser.add_argument('--output', type=str, default=str(EXTRACTED_DIR),
                       help='Base output directory for organized assets')
    parser.add_argument('--overwrite', action='store_true',
                       help='Overwrite existing organized assets')
    parser.add_argument('--extraction-manifest', type=str, default=str(MANIFEST_FILE),
                       help='Path to extraction manifest (for reference)')
    
    args = parser.parse_args()
    
    input_dir = Path(args.input)
    output_base = Path(args.output)
    
    if not input_dir.exists():
        print(f"Error: Input directory does not exist: {input_dir}")
        print(f"💡 Tip: Run extract_modpack_assets.py first")
        sys.exit(1)
    
    output_base.mkdir(parents=True, exist_ok=True)
    
    organizer = AssetOrganizer(input_dir, output_base, args.overwrite)
    organizer.organize_all()
    organizer.save_manifest()
    organizer.print_summary()
    
    print("\n" + "="*60)
    print("Organization Complete!")
    print("="*60)
    print(f"\nNext steps:")
    print(f"1. Review organized assets in: {output_base}")
    print(f"2. Run optimization scripts:")
    print(f"   - python optimize_textures.py (texture atlasing)")
    print(f"   - python convert_models.py (model conversion)")

if __name__ == '__main__':
    main()

