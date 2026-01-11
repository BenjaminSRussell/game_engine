#!/usr/bin/env python3
"""
Rename script to change 'minecraft' references to 'block game' throughout the project.
This script handles:
- Directory renaming
- File renaming
- Content replacement in all source files
"""

import os
import re
import shutil
from pathlib import Path

PROJECT_ROOT = Path(__file__).parent

# Mapping of old to new names
REPLACEMENTS = [
    # Project/Bundle names
    ("com.benjaminrussell.MinecraftV2", "com.benjaminrussell.BlockGame"),
    ("MinecraftV2", "BlockGame"),
    ("MinecraftApp", "BlockGameApp"),
    ("minecraft_v2", "block_game"),
    ("minecraftv2", "blockgame"),
    ("Minecraft v2", "Block Game"),
    ("Minecraft V2", "Block Game"),
    ("Minecraft", "BlockGame"),
    ("minecraft", "blockgame"),
    # C identifiers and functions
    ("MinecraftV2State", "BlockGameState"),
    ("minecraft_v2_init", "block_game_init"),
    ("minecraft_v2_update", "block_game_update"),
    ("minecraft_v2_render", "block_game_render"),
    ("minecraft_v2_module", "block_game_module"),
    ("Minecraft-Bridging-Header", "BlockGame-Bridging-Header"),
]

# Directories to rename
DIRS_TO_RENAME = [
    "src/game/minecraftv2",
]

# File patterns to process
PROCESS_PATTERNS = [
    "*.c",
    "*.h",
    "*.cpp",
    "*.hpp",
    "*.m",
    "*.swift",
    "*.cmake",
    "CMakeLists.txt",
    "CMakeCache.txt",
    "*.md",
    "*.txt",
    "Package.swift",
]

# Directories to exclude from processing
EXCLUDE_DIRS = {
    ".git",
    "CMakeFiles",
    "build",
    ".build",
    "Vendor",
}

def should_exclude(path):
    """Check if a path should be excluded from processing."""
    parts = Path(path).parts
    return any(part in EXCLUDE_DIRS for part in parts)

def rename_directories():
    """Rename directories from old to new names."""
    print("\n=== Renaming Directories ===")
    for old_dir in DIRS_TO_RENAME:
        old_path = PROJECT_ROOT / old_dir
        if old_path.exists():
            # Determine new directory name
            new_dir = old_dir
            for old_name, new_name in REPLACEMENTS:
                new_dir = new_dir.replace(old_name, new_name)

            new_path = PROJECT_ROOT / new_dir
            print(f"Renaming: {old_path} -> {new_path}")
            shutil.move(str(old_path), str(new_path))
        else:
            print(f"Directory not found: {old_path}")

def rename_files():
    """Rename files matching old names."""
    print("\n=== Renaming Files ===")
    for root, dirs, files in os.walk(PROJECT_ROOT):
        # Remove excluded directories from traversal
        dirs[:] = [d for d in dirs if not should_exclude(os.path.join(root, d))]

        for filename in files:
            new_filename = filename
            for old_name, new_name in REPLACEMENTS:
                new_filename = new_filename.replace(old_name, new_name)

            if new_filename != filename:
                old_path = os.path.join(root, filename)
                new_path = os.path.join(root, new_filename)
                print(f"Renaming file: {filename} -> {new_filename}")
                os.rename(old_path, new_path)

def replace_in_files():
    """Replace old names with new names in file contents."""
    print("\n=== Replacing Content in Files ===")
    file_count = 0
    replacement_count = 0

    for root, dirs, files in os.walk(PROJECT_ROOT):
        # Remove excluded directories from traversal
        dirs[:] = [d for d in dirs if not should_exclude(os.path.join(root, d))]

        for filename in files:
            # Check if file matches a processing pattern
            should_process = any(
                Path(filename).match(pattern) for pattern in PROCESS_PATTERNS
            )

            if not should_process:
                continue

            filepath = os.path.join(root, filename)
            try:
                with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
                    content = f.read()

                original_content = content

                # Apply replacements - order matters! Do longer replacements first
                sorted_replacements = sorted(REPLACEMENTS, key=lambda x: len(x[0]), reverse=True)
                for old_name, new_name in sorted_replacements:
                    # Use word boundaries to avoid partial replacements in unexpected places
                    # but allow for common patterns
                    pattern = r'\b' + re.escape(old_name) + r'\b'
                    content = re.sub(pattern, new_name, content)

                if content != original_content:
                    with open(filepath, 'w', encoding='utf-8') as f:
                        f.write(content)
                    file_count += 1
                    replacement_count += content.count(new_name)
                    print(f"Updated: {os.path.relpath(filepath, PROJECT_ROOT)}")

            except Exception as e:
                print(f"Error processing {filepath}: {e}")

    print(f"\nProcessed {file_count} files")

def main():
    """Main entry point."""
    print("Starting rename from 'minecraft' to 'block game'...")
    print(f"Project root: {PROJECT_ROOT}")

    # Step 1: Rename directories
    rename_directories()

    # Step 2: Rename files
    rename_files()

    # Step 3: Replace in files
    replace_in_files()

    print("\n=== Renaming Complete ===")
    print("Please verify the changes and rebuild the project:")
    print("  cmake .")
    print("  make")

if __name__ == "__main__":
    main()
