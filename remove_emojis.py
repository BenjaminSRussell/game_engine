#!/usr/bin/env python3
"""
Script to remove all emojis from source code files.
Supports various file types including .c, .h, .swift, .md, .txt, .py, .m, .cmake
"""

import os
import re
import sys
from pathlib import Path

# Emoji regex pattern - covers all Unicode emoji ranges
EMOJI_PATTERN = re.compile(
    r'[\U0001F600-\U0001F64F]|'  # Emoticons
    r'[\U0001F300-\U0001F5FF]|'  # Misc Symbols and Pictographs
    r'[\U0001F680-\U0001F6FF]|'  # Transport and Map Symbols
    r'[\U0001F1E0-\U0001F1FF]|'  # Flags (iOS)
    r'[\U00002600-\U000027BF]|'  # Misc symbols
    r'[\U0001F900-\U0001F9FF]|'  # Supplemental Symbols and Pictographs
    r'[\U0001FA70-\U0001FAFF]|'  # Symbols and Pictographs Extended-A
    r'[\U00002702-\U000027B0]|'  # Dingbats
    r'[\U000024C2-\U0001F251]',  # Enclosed characters
    flags=re.UNICODE
)

# File extensions to process
SOURCE_EXTENSIONS = {
    '.c', '.h', '.cpp', '.hpp', '.cc', '.cxx',
    '.swift', '.m', '.mm',
    '.py', '.js', '.ts', '.jsx', '.tsx',
    '.md', '.txt', '.rst',
    '.cmake', '.yml', '.yaml', '.json',
    '.sh', '.bash', '.zsh',
    '.cfg', '.conf', '.ini', '.toml'
}

def should_process_file(file_path):
    """Check if file should be processed based on extension and location."""
    if file_path.is_dir():
        return False
    
    # Skip hidden files and directories
    if any(part.startswith('.') for part in file_path.parts):
        return False
    
    # Skip .git directory
    if '.git' in file_path.parts:
        return False
    
    # Check extension
    return file_path.suffix.lower() in SOURCE_EXTENSIONS

def remove_emojis_from_file(file_path):
    """Remove emojis from a single file."""
    try:
        with open(file_path, 'r', encoding='utf-8', errors='replace') as f:
            content = f.read()
        
        # Count emojis before removal
        emoji_count = len(EMOJI_PATTERN.findall(content))
        
        if emoji_count > 0:
            # Remove emojis
            content_without_emojis = EMOJI_PATTERN.sub('', content)
            
            # Write back to file
            with open(file_path, 'w', encoding='utf-8') as f:
                f.write(content_without_emojis)
            
            return emoji_count
        
        return 0
    
    except Exception as e:
        print(f"Error processing {file_path}: {e}")
        return 0

def process_directory(directory_path, dry_run=False):
    """Process all files in a directory recursively."""
    total_emojis = 0
    files_processed = 0
    
    directory_path = Path(directory_path)
    
    for file_path in directory_path.rglob('*'):
        if should_process_file(file_path):
            if dry_run:
                with open(file_path, 'r', encoding='utf-8', errors='replace') as f:
                    content = f.read()
                emoji_count = len(EMOJI_PATTERN.findall(content))
                if emoji_count > 0:
                    print(f"Would remove {emoji_count} emojis from: {file_path}")
                    total_emojis += emoji_count
                    files_processed += 1
            else:
                emoji_count = remove_emojis_from_file(file_path)
                if emoji_count > 0:
                    print(f"Removed {emoji_count} emojis from: {file_path}")
                    total_emojis += emoji_count
                    files_processed += 1
    
    return total_emojis, files_processed

def main():
    """Main function."""
    if len(sys.argv) < 2:
        print("Usage: python remove_emojis.py <directory> [--dry-run]")
        sys.exit(1)
    
    directory = sys.argv[1]
    dry_run = '--dry-run' in sys.argv
    
    if not os.path.exists(directory):
        print(f"Error: Directory '{directory}' does not exist")
        sys.exit(1)
    
    print(f"{'Scanning' if dry_run else 'Processing'} directory: {directory}")
    
    total_emojis, files_processed = process_directory(directory, dry_run)
    
    if dry_run:
        print(f"\nDry run complete: Would remove {total_emojis} emojis from {files_processed} files")
    else:
        print(f"\nComplete: Removed {total_emojis} emojis from {files_processed} files")

if __name__ == '__main__':
    main()
