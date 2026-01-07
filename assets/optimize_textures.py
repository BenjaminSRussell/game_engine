#!/usr/bin/env python3
"""
Texture Optimization Script

Creates texture atlases from individual textures to reduce draw calls.
Also optimizes texture formats and resolutions.

Usage:
    python optimize_textures.py [--input extracted] [--output optimized/textures] [--atlas-size 4096]
"""

import argparse
import json
import sys
from pathlib import Path
from PIL import Image, ImageOps
import math

SCRIPT_DIR = Path(__file__).parent
ASSETS_DIR = SCRIPT_DIR
EXTRACTED_DIR = ASSETS_DIR / "extracted"
OPTIMIZED_DIR = ASSETS_DIR / "optimized"
TEXTURE_OPTIMIZED_DIR = OPTIMIZED_DIR / "textures"

class TextureAtlasBuilder:
    def __init__(self, atlas_size: int = 4096):
        self.atlas_size = atlas_size
        self.atlases = {}  # category -> (image, metadata)
    
    def pack_textures(self, texture_files: list, category: str, subcategory: str = "atlas") -> tuple:
        """
        Pack textures into an atlas using simple bin-packing algorithm.
        
        Args:
            texture_files: List of texture file paths
            category: Category name (vehicles, magic, etc.)
            subcategory: Subcategory name for the atlas
            
        Returns:
            Tuple of (atlas_image, atlas_metadata_dict)
        """
        # Load and prepare textures
        textures = []
        for tex_path in texture_files:
            try:
                img = Image.open(tex_path)
                # Ensure power-of-two dimensions (optional: resize)
                img = self.ensure_power_of_two(img)
                textures.append({
                    'path': tex_path,
                    'image': img,
                    'width': img.width,
                    'height': img.height,
                    'name': tex_path.stem
                })
            except Exception as e:
                print(f"  ⚠ Failed to load {tex_path.name}: {e}")
                continue
        
        if not textures:
            return None, None
        
        # Simple bin-packing: arrange in rows
        atlas_image = Image.new('RGBA', (self.atlas_size, self.atlas_size), (0, 0, 0, 0))
        metadata = {
            'atlas_size': self.atlas_size,
            'textures': [],
            'category': category,
            'subcategory': subcategory
        }
        
        x, y = 0, 0
        row_height = 0
        
        for tex_data in textures:
            img = tex_data['image']
            w, h = img.width, img.height
            
            # Check if we need a new row
            if x + w > self.atlas_size:
                x = 0
                y += row_height
                row_height = 0
            
            # Check if we've run out of space
            if y + h > self.atlas_size:
                print(f"  ⚠ Atlas full, skipping remaining textures")
                break
            
            # Paste texture into atlas
            atlas_image.paste(img, (x, y))
            
            # Record metadata
            metadata['textures'].append({
                'name': tex_data['name'],
                'source_path': str(tex_data['path']),
                'x': x,
                'y': y,
                'width': w,
                'height': h
            })
            
            x += w
            row_height = max(row_height, h)
        
        return atlas_image, metadata
    
    def ensure_power_of_two(self, img: Image.Image) -> Image.Image:
        """
        Ensure image dimensions are power of two (resize if necessary).
        
        Args:
            img: PIL Image
            
        Returns:
            Resized PIL Image
        """
        def next_power_of_two(n):
            return 2 ** math.ceil(math.log2(max(1, n)))
        
        w, h = img.width, img.height
        new_w = next_power_of_two(w)
        new_h = next_power_of_two(h)
        
        if new_w != w or new_h != h:
            # Resize using high-quality resampling
            img = img.resize((new_w, new_h), Image.Resampling.LANCZOS)
        
        return img

class TextureOptimizer:
    def __init__(self, input_dir: Path, output_dir: Path, atlas_size: int = 4096):
        self.input_dir = Path(input_dir)
        self.output_dir = Path(output_dir)
        self.atlas_builder = TextureAtlasBuilder(atlas_size)
        self.stats = {
            'textures_processed': 0,
            'atlases_created': 0,
            'textures_optimized': 0,
            'errors': 0
        }
    
    def find_texture_files(self, directory: Path) -> list:
        """Find all texture files in directory recursively."""
        textures = []
        for ext in ['.png', '.jpg', '.jpeg']:
            textures.extend(directory.rglob(f'*{ext}'))
        return textures
    
    def create_category_atlases(self) -> None:
        """Create texture atlases for each category."""
        print("\n" + "="*60)
        print("Creating Texture Atlases")
        print("="*60)
        
        categories = ['vehicles', 'magic', 'tech', 'mobs', 'dimensions', 'environment']
        
        for category in categories:
            category_dir = self.input_dir / category
            if not category_dir.exists():
                continue
            
            print(f"\n📦 Processing category: {category}")
            
            # Find all textures in this category
            texture_files = self.find_texture_files(category_dir)
            
            if not texture_files:
                print(f"  ⚠ No textures found")
                continue
            
            print(f"  Found {len(texture_files)} texture(s)")
            
            # Group by subdirectory (mod name)
            textures_by_mod = {}
            for tex_path in texture_files:
                # Get relative path from category
                rel_path = tex_path.relative_to(category_dir)
                mod_name = rel_path.parts[0] if len(rel_path.parts) > 1 else 'misc'
                
                if mod_name not in textures_by_mod:
                    textures_by_mod[mod_name] = []
                textures_by_mod[mod_name].append(tex_path)
            
            # Create atlas for each mod
            for mod_name, mod_textures in textures_by_mod.items():
                print(f"\n  [{mod_name}]")
                print(f"    Packing {len(mod_textures)} texture(s) into atlas...")
                
                atlas_img, atlas_meta = self.atlas_builder.pack_textures(
                    mod_textures, category, mod_name
                )
                
                if atlas_img and atlas_meta:
                    # Save atlas
                    atlas_dir = self.output_dir / "atlases" / category
                    atlas_dir.mkdir(parents=True, exist_ok=True)
                    atlas_path = atlas_dir / f"{mod_name}_atlas.png"
                    atlas_img.save(atlas_path, optimize=True)
                    
                    # Save metadata
                    meta_path = atlas_dir / f"{mod_name}_atlas.json"
                    with open(meta_path, 'w') as f:
                        json.dump(atlas_meta, f, indent=2)
                    
                    self.stats['atlases_created'] += 1
                    print(f"    ✓ Created atlas: {atlas_path.name}")
                    print(f"      Packed {len(atlas_meta['textures'])} texture(s)")
    
    def optimize_individual_textures(self) -> None:
        """Optimize individual textures (resize, compress, etc.)."""
        print("\n" + "="*60)
        print("Optimizing Individual Textures")
        print("="*60)
        
        # Copy and optimize textures that aren't in atlases
        # (e.g., UI textures, entity textures that need separate handling)
        texture_files = self.find_texture_files(self.input_dir)
        
        optimized_dir = self.output_dir / "individual"
        optimized_dir.mkdir(parents=True, exist_ok=True)
        
        for tex_path in texture_files:
            try:
                # Preserve directory structure
                rel_path = tex_path.relative_to(self.input_dir)
                dest_path = optimized_dir / rel_path
                dest_path.parent.mkdir(parents=True, exist_ok=True)
                
                # Load, optimize, and save
                img = Image.open(tex_path)
                img = ImageOps.exif_transpose(img)  # Fix orientation
                
                # Convert to RGBA if needed
                if img.mode != 'RGBA':
                    img = img.convert('RGBA')
                
                # Save optimized
                dest_path = dest_path.with_suffix('.png')  # Convert to PNG
                img.save(dest_path, 'PNG', optimize=True)
                
                self.stats['textures_optimized'] += 1
                
            except Exception as e:
                print(f"  ⚠ Failed to optimize {tex_path}: {e}")
                self.stats['errors'] += 1
    
    def print_summary(self) -> None:
        """Print optimization summary."""
        print("\n" + "="*60)
        print("Optimization Summary")
        print("="*60)
        print(f"Atlases created: {self.stats['atlases_created']}")
        print(f"Textures optimized: {self.stats['textures_optimized']}")
        print(f"Errors: {self.stats['errors']}")

def main():
    parser = argparse.ArgumentParser(description='Optimize and atlas textures from extracted mods')
    parser.add_argument('--input', type=str, default=str(EXTRACTED_DIR),
                       help='Input directory containing organized assets')
    parser.add_argument('--output', type=str, default=str(TEXTURE_OPTIMIZED_DIR),
                       help='Output directory for optimized textures')
    parser.add_argument('--atlas-size', type=int, default=4096,
                       help='Atlas size (power of 2, e.g., 4096)')
    parser.add_argument('--atlases-only', action='store_true',
                       help='Only create atlases, skip individual optimization')
    parser.add_argument('--individual-only', action='store_true',
                       help='Only optimize individual textures, skip atlases')
    
    args = parser.parse_args()
    
    input_dir = Path(args.input)
    output_dir = Path(args.output)
    
    if not input_dir.exists():
        print(f"Error: Input directory does not exist: {input_dir}")
        print(f"💡 Tip: Run organize_extracted_assets.py first")
        sys.exit(1)
    
    output_dir.mkdir(parents=True, exist_ok=True)
    
    optimizer = TextureOptimizer(input_dir, output_dir, args.atlas_size)
    
    if not args.individual_only:
        optimizer.create_category_atlases()
    
    if not args.atlases_only:
        optimizer.optimize_individual_textures()
    
    optimizer.print_summary()
    
    print("\n" + "="*60)
    print("Texture Optimization Complete!")
    print("="*60)

if __name__ == '__main__':
    try:
        from PIL import Image, ImageOps
    except ImportError:
        print("Error: PIL (Pillow) is required. Install with: pip install Pillow")
        sys.exit(1)
    
    main()

