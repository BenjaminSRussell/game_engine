#!/usr/bin/env python3
"""
Modpack Downloader Script

Downloads Minecraft modpacks and standalone mods from configured sources.
Supports direct URL downloads and can be extended for CurseForge/Modrinth API integration.

Usage:
    python download_modpacks.py [--config config.json] [--modpack name] [--category vehicles|magic|tech|mobs|dimensions]
"""

import argparse
import json
import os
import sys
import time
import hashlib
import requests
from pathlib import Path
from urllib.parse import urlparse
from typing import Optional, Dict, List

# Configuration
SCRIPT_DIR = Path(__file__).parent
ASSETS_DIR = SCRIPT_DIR
DOWNLOADS_DIR = ASSETS_DIR / "downloads"
MODPACKS_DIR = DOWNLOADS_DIR / "modpacks"
STANDALONE_DIR = DOWNLOADS_DIR / "standalone_mods"
CONFIG_FILE = DOWNLOADS_DIR / "modpack_config.json"

class ModpackDownloader:
    def __init__(self, config_path: Path):
        self.config_path = config_path
        self.config = self.load_config()
        self.session = requests.Session()
        self.session.headers.update({
            'User-Agent': self.config['download_settings']['user_agent']
        })
    
    def load_config(self) -> dict:
        """Load configuration from JSON file."""
        if not self.config_path.exists():
            print(f"Error: Config file not found: {self.config_path}")
            sys.exit(1)
        
        with open(self.config_path, 'r') as f:
            return json.load(f)
    
    def download_file(self, url: str, dest_path: Path, max_retries: int = 3) -> bool:
        """
        Download a file from URL with progress indication.
        
        Args:
            url: URL to download from
            dest_path: Destination file path
            max_retries: Maximum number of retry attempts
            
        Returns:
            True if download succeeded, False otherwise
        """
        if not url:
            print(f"  ⚠ Skipping: No download URL provided")
            return False
        
        if dest_path.exists():
            print(f"  ✓ Already exists: {dest_path.name}")
            return True
        
        print(f"  📥 Downloading: {url}")
        
        for attempt in range(max_retries):
            try:
                response = self.session.get(url, stream=True, timeout=300)
                response.raise_for_status()
                
                total_size = int(response.headers.get('content-length', 0))
                downloaded = 0
                
                dest_path.parent.mkdir(parents=True, exist_ok=True)
                
                with open(dest_path, 'wb') as f:
                    for chunk in response.iter_content(chunk_size=8192):
                        if chunk:
                            f.write(chunk)
                            downloaded += len(chunk)
                            if total_size > 0:
                                percent = (downloaded / total_size) * 100
                                print(f"\r    Progress: {percent:.1f}% ({downloaded}/{total_size} bytes)", end='', flush=True)
                
                print()  # New line after progress
                print(f"  ✓ Downloaded: {dest_path.name} ({self.format_size(downloaded)})")
                return True
                
            except requests.exceptions.RequestException as e:
                if attempt < max_retries - 1:
                    wait_time = 2 ** attempt
                    print(f"  ⚠ Attempt {attempt + 1} failed: {e}. Retrying in {wait_time}s...")
                    time.sleep(wait_time)
                else:
                    print(f"  ✗ Failed after {max_retries} attempts: {e}")
                    if dest_path.exists():
                        dest_path.unlink()
                    return False
        
        return False
    
    def format_size(self, size_bytes: int) -> str:
        """Format file size in human-readable format."""
        for unit in ['B', 'KB', 'MB', 'GB']:
            if size_bytes < 1024.0:
                return f"{size_bytes:.1f} {unit}"
            size_bytes /= 1024.0
        return f"{size_bytes:.1f} TB"
    
    def download_modpacks(self, filter_name: Optional[str] = None) -> None:
        """Download modpacks from configuration."""
        print("\n" + "="*60)
        print("Downloading Modpacks")
        print("="*60)
        
        modpacks = self.config['modpacks']
        
        # Sort by priority
        modpacks = sorted(modpacks, key=lambda x: x['priority'])
        
        # Filter if name specified
        if filter_name:
            modpacks = [m for m in modpacks if filter_name.lower() in m['name'].lower()]
            if not modpacks:
                print(f"⚠ No modpack found matching: {filter_name}")
                return
        
        print(f"\nFound {len(modpacks)} modpack(s) to process\n")
        
        for modpack in modpacks:
            name = modpack['name']
            url = modpack.get('download_url')
            
            print(f"\n[{name}]")
            print(f"  Description: {modpack.get('description', 'N/A')}")
            
            if url:
                # Determine file extension from URL or default to .zip
                parsed = urlparse(url)
                ext = os.path.splitext(parsed.path)[1] or '.zip'
                dest_path = MODPACKS_DIR / f"{name}{ext}"
                
                self.download_file(url, dest_path)
            else:
                print(f"  ⚠ No download URL configured. Please add 'download_url' to config.")
                print(f"  💡 Tip: Download manually and place in: {MODPACKS_DIR}")
    
    def download_standalone_mods(self, category: Optional[str] = None) -> None:
        """Download standalone mods by category."""
        print("\n" + "="*60)
        print("Downloading Standalone Mods")
        print("="*60)
        
        standalone = self.config['standalone_mods']
        
        categories_to_download = [category] if category else standalone.keys()
        
        for cat in categories_to_download:
            if cat not in standalone:
                print(f"⚠ Unknown category: {cat}")
                continue
            
            print(f"\n[{cat.upper()}]")
            mods = standalone[cat]
            
            cat_dir = STANDALONE_DIR / cat
            cat_dir.mkdir(parents=True, exist_ok=True)
            
            for mod in mods:
                name = mod['name']
                url = mod.get('download_url')
                
                print(f"\n  [{name}]")
                print(f"    Description: {mod.get('description', 'N/A')}")
                
                if url:
                    # Determine file extension from URL
                    parsed = urlparse(url)
                    ext = os.path.splitext(parsed.path)[1] or '.jar'
                    dest_path = cat_dir / f"{name}{ext}"
                    
                    self.download_file(url, dest_path)
                else:
                    print(f"    ⚠ No download URL configured")
    
    def list_available(self) -> None:
        """List all configured modpacks and mods."""
        print("\n" + "="*60)
        print("Available Modpacks & Mods")
        print("="*60)
        
        print("\n📦 MODPACKS:")
        for modpack in sorted(self.config['modpacks'], key=lambda x: x['priority']):
            status = "✓ URL configured" if modpack.get('download_url') else "⚠ No URL"
            print(f"  [{modpack['priority']:2d}] {modpack['name']:30s} - {status}")
            print(f"      {modpack.get('description', 'N/A')}")
        
        print("\n🔧 STANDALONE MODS:")
        for category, mods in self.config['standalone_mods'].items():
            print(f"\n  {category.upper()}:")
            for mod in mods:
                status = "✓ URL configured" if mod.get('download_url') else "⚠ No URL"
                print(f"    - {mod['name']:30s} - {status}")

def main():
    parser = argparse.ArgumentParser(description='Download Minecraft modpacks and mods')
    parser.add_argument('--config', type=str, default=str(CONFIG_FILE),
                       help='Path to configuration file')
    parser.add_argument('--modpack', type=str, help='Download specific modpack by name')
    parser.add_argument('--category', type=str, choices=['vehicles', 'magic', 'tech', 'mobs', 'dimensions'],
                       help='Download mods from specific category')
    parser.add_argument('--list', action='store_true', help='List all available modpacks and mods')
    parser.add_argument('--standalone-only', action='store_true', help='Download only standalone mods')
    parser.add_argument('--modpacks-only', action='store_true', help='Download only modpacks')
    
    args = parser.parse_args()
    
    # Ensure directories exist
    MODPACKS_DIR.mkdir(parents=True, exist_ok=True)
    STANDALONE_DIR.mkdir(parents=True, exist_ok=True)
    
    downloader = ModpackDownloader(Path(args.config))
    
    if args.list:
        downloader.list_available()
        return
    
    if args.modpacks_only or not args.standalone_only:
        downloader.download_modpacks(filter_name=args.modpack)
    
    if args.standalone_only or not args.modpacks_only:
        downloader.download_standalone_mods(category=args.category)
    
    print("\n" + "="*60)
    print("Download Complete!")
    print("="*60)
    print(f"\nNext steps:")
    print(f"1. Run extraction script: python extract_modpack_assets.py")
    print(f"2. Organize assets: python organize_extracted_assets.py")

if __name__ == '__main__':
    main()

