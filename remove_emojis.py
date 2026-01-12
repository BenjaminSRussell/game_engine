#!/usr/bin/env python3

"""
Emoji Removal Script
This script removes all emojis from the codebase while preserving functionality.
"""

import os
import re
import sys
from pathlib import Path

# Common emoji patterns to remove
EMOJI_PATTERNS = [
    # Success/Completion emojis
    r'[\u2705\U0001F3FB\U0001F3FD]',  # 
    r'[\u2705\U0001F3FB\U0001F3FD]',  #  (variant)
    r'[\u2728]',  #  (check mark)
    r'[\ud83c\udf89]',  #  (heavy check mark)
    r'[\ud83c\udf8f]',  #  (negative squared cross mark)
    r'[\ud83d\udcc8]',  #  (white heavy check mark)
    r'[\u2611]',  #  (ballot box with check)
    
    # Error/Failure emojis
    r'[\u274c]',  # 
    r'[\ud83d\udd34]',  #  (cross mark)
    r'[\u2716]',  #  (cross mark)
    
    # Warning emojis
    r'[\u26a0\ufe0f]',  # 
    r'[\u26a0]',  #  (warning sign)
    r'[\ud83d\udea7]',  #  (warning sign variant)
    
    # Information/Search emojis
    r'[\ud83d\udd0d]',  # 
    r'[\ud83d\udd0d\ufe0f]',  #  (magnifying glass tilted left)
    r'[\ud83d\udd0d\ud83c\udf7f]',  #  (magnifying glass tilted left)
    
    # Tool/Build emojis
    r'[\ud83d\udd27]',  # 
    r'[\ud83d\udd28]',  #  (hammer and wrench)
    r'[\ud83d\udee0\ufe0f]',  # 
    r'[\ud83d\udee0]',  #  (hammer and wrench)
    r'[\ud83d\udcbb]',  # 
    r'[\ud83d\udcbc]',  # 
    
    # Link/Connection emojis
    r'[\ud83d\udd17]',  # 
    r'[\ud83d\udd17\ufe0f]',  #  (link variant)
    
    # Test/Science emojis
    r'[\ud83e\uddea]',  # 
    r'[\ud83d\udc68]',  #  (test tube)
    r'[\ud83d\udc69]',  #  (petri dish)
    
    # Documentation/Book emojis
    r'[\ud83d\udcda]',  # 
    r'[\ud83d\udcd6]',  #  (open book)
    r'[\ud83d\udcd4]',  #  (notebook)
    r'[\ud83d\udcd5]',  #  (notebook with decorative cover)
    r'[\ud83d\udcd7]',  #  (notebook)
    r'[\ud83d\udcd1]',  #  (bookmark tabs)
    r'[\ud83d\udcc4]',  #  (page facing up)
    r'[\ud83d\udcc3]',  #  (page facing down)
    r'[\ud83d\udcd2]',  #  (bookmark)
    r'[\ud83d\udcd0]',  #  (bookmark tabs)
    
    # Chart/Graph emojis
    r'[\ud83d\udcca]',  # 
    r'[\ud83d\udcc8]',  #  (chart increasing)
    r'[\ud83d\udcc9]',  #  (chart decreasing)
    r'[\ud83d\udcc7]',  #  (bar chart)
    r'[\ud83d\udcc6]',  #  (page curl up)
    r'[\ud83d\udcc5]',  #  (page curl down)
    
    # Target/Goal emojis
    r'[\ud83c\udfaf]',  # 
    r'[\ud83c\udfaf\ufe0f]',  #  (direct hit)
    
    # Rocket/Launch emojis
    r'[\ud83d\ude80]',  # 
    r'[\ud83d\ude80\ufe0f]',  #  (rocket variant)
    
    # Trophy/Achievement emojis
    r'[\ud83c\udfc6]',  # 
    r'[\ud83c\udfc5]',  #  (military medal)
    r'[\ud83c\udfc6\ufe0f]',  #  (trophy variant)
    
    # Celebration emojis
    r'[\ud83c\udf89]',  # 
    r'[\ud83c\udf89\ufe0f]',  #  (party popper variant)
    r'[\ud83c\udf8a]',  #  (confetti ball)
    r'[\ud83c\udf8a\ufe0f]',  #  (confetti ball variant)
    r'[\ud83c\udf86]',  #  (fireworks)
    r'[\ud83c\udf86\ufe0f]',  #  (fireworks variant)
    
    # Package/Box emojis
    r'[\ud83d\udce6]',  # 
    r'[\ud83d\udce6\ufe0f]',  #  (package variant)
    
    # Refresh/Sync emojis
    r'[\ud83d\udd04]',  # 
    r'[\ud83d\udd04\ufe0f]',  #  (counterclockwise arrows)
    r'[\ud83d\udd03]',  # 
    r'[\ud83d\udd03\ufe0f]',  #  (clockwise vertical arrows)
    
    # Game/Play emojis
    r'[\ud83c\udfae]',  # 
    r'[\ud83c\udfae\ufe0f]',  #  (video game variant)
    r'[\ud83c\udfaf]',  #  (direct hit)
    
    # Film/Movie emojis
    r'[\ud83c\udfac]',  # 
    r'[\ud83c\udfac\ufe0f]',  #  (clapper board variant)
    
    # Music/Sound emojis
    r'[\ud83c\udfb5]',  # 
    r'[\ud83c\udfb5\ufe0f]',  #  (musical note variant)
    r'[\ud83c\udfb6]',  #  (multiple musical notes)
    r'[\ud83c\udfb7]',  #  (saxophone)
    r'[\ud83c\udfb8]',  #  (guitar)
    r'[\ud83c\udfb9]',  #  (musical keyboard)
    r'[\ud83c\udfba]',  #  (trumpet)
    r'[\ud83c\udfbb]',  #  (violin)
    
    # Wave/Sound emojis
    r'[\ud83d\udc0a]',  # 
    r'[\ud83d\udc0a\ufe0f]',  #  (speaker high volume)
    r'[\ud83d\udc08]',  #  (speaker medium volume)
    r'[\ud83d\udc09]',  #  (speaker low volume)
    r'[\ud83d\udd07]',  #  (bell)
    r'[\ud83d\udd07\ufe0f]',  #  (bell variant)
    
    # Lightning/Power emojis
    r'[\u26a1]',  # 
    r'[\u26a1\ufe0f]',  #  (high voltage variant)
    r'[\ud83d\udca1]',  # 
    r'[\ud83d\udca1\ufe0f]',  #  (light bulb variant)
    
    # Art/Palette emojis
    r'[\ud83c\udfa8]',  # 
    r'[\ud83c\udfa8\ufe0f]',  #  (artist palette variant)
    
    # Progress/Waiting emojis
    r'[\u23f3]',  # 
    r'[\u23f3\ufe0f]',  #  (hourglass with flowing sand)
    r'[\u23f0]',  # 
    r'[\u23f0\ufe0f]',  #  (alarm clock)
    r'[\u23f1]',  # 
    r'[\u23f1\ufe0f]',  #  (stopwatch)
    
    # Clock/Time emojis
    r'[\ud83d\udd50]',  # 
    r'[\ud83d\udd50\ufe0f]',  #  (bell variant)
    r'[\ud83d\udd5a]',  # 
    r'[\ud83d\udd5a\ufe0f]',  #  (new moon face)
    
    # Gear/Settings emojis
    r'[\u2699]',  # 
    r'[\u2699\ufe0f]',  #  (gear variant)
    r'[\ud83d\udd27]',  #  (wrench)
    
    # Document/Write emojis
    r'[\ud83d\udcdd]',  # 
    r'[\ud83d\udcdd\ufe0f]',  #  (memo variant)
    r'[\u270d\ufe0f]',  # 
    r'[\u270f]',  #  (pencil)
    r'[\ud83d\udd8a]',  # 
    r'[\ud83d\udd8a\ufe0f]',  #  (pencil variant)
    r'[\ud83d\udd8b]',  # 
    r'[\ud83d\udd8c]',  # 
    r'[\ud83d\udd8d]',  # 
    r'[\ud83d\udd8e]',  # 
    r'[\ud83d\udd8f]',  # 
    r'[\ud83d\udd90]',  # 
    r'[\ud83d\udd91]',  # 
    r'[\ud83d\udd92]',  # 
    r'[\ud83d\udd93]',  # 
    r'[\ud83d\udd94]',  # 
    r'[\ud83d\udd95]',  # 
    r'[\ud83d\udd96]',  # 
    r'[\ud83d\udd97]',  # 
    r'[\ud83d\udd98]',  # 
    r'[\ud83d\udd99]',  # 
    r'[\ud83d\udd9a]',  # 
    r'[\ud83d\udd9b]',  # 
    r'[\ud83d\udd9c]',  # 
    r'[\ud83d\udd9d]',  # 
    r'[\ud83d\udd9e]',  # 
    r'[\ud83d\udd9f]',  # 
    r'[\ud83d\udda0]',  # 
    r'[\ud83d\udda1]',  # 
    r'[\ud83d\udda2]',  # 
    r'[\ud83d\udda3]',  # 
    r'[\ud83d\udda4]',  # 
    r'[\ud83d\udda5]',  # 
    r'[\ud83d\udda6]',  # 
    r'[\ud83d\udda7]',  # 
    r'[\ud83d\udda8]',  # 
    r'[\ud83d\udda9]',  # 
    r'[\ud83d\uddaa]',  # 
    r'[\ud83d\uddab]',  # 
    r'[\ud83d\uddac]',  # 
    r'[\ud83d\uddad]',  # 
    r'[\ud83d\uddae]',  # 
    r'[\ud83d\uddaf]',  # 
    r'[\ud83d\uddb0]',  # 
    r'[\ud83d\uddb1]',  # 
    r'[\ud83d\uddb2]',  # 
    r'[\ud83d\uddb3]',  # 
    r'[\ud83d\uddb4]',  # 
    r'[\ud83d\uddb5]',  # 
    r'[\ud83d\uddb6]',  # 
    r'[\ud83d\uddb7]',  # 
    r'[\ud83d\uddb8]',  # 
    r'[\ud83d\uddb9]',  # 
    r'[\ud83d\uddba]',  # 
    r'[\ud83d\uddbb]',  # 
    r'[\ud83d\uddbc]',  # 
    r'[\ud83d\uddbd]',  # 
    r'[\ud83d\uddbe]',  # 
    r'[\ud83d\uddbf]',  # 
]

def remove_emojis_from_file(filepath):
    """Remove emojis from a single file."""
    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            content = f.read()
        
        original_content = content
        
        # Apply all emoji patterns
        for pattern in EMOJI_PATTERNS:
            content = re.sub(pattern, '', content)
        
        # Also remove any remaining non-ASCII characters that might be emojis
        content = re.sub(r'[^\x00-\x7F]', '', content)
        
        # Only write if content changed
        if content != original_content:
            with open(filepath, 'w', encoding='utf-8') as f:
                f.write(content)
            return True
        return False
        
    except Exception as e:
        print(f"Error processing {filepath}: {e}")
        return False

def process_directory(directory):
    """Process all files in a directory recursively."""
    changed_files = []
    total_files = 0
    
    for filepath in Path(directory).rglob('*'):
        # Skip binary files and directories
        if filepath.is_dir():
            continue
            
        # Skip certain file types that shouldn't contain emojis
        if filepath.suffix.lower() in [
            '.png', '.jpg', '.jpeg', '.gif', '.bmp', '.ico',
            '.mp3', '.wav', '.ogg', '.flac',
            '.mp4', '.avi', '.mov', '.mkv',
            '.zip', '.tar', '.gz', '.rar',
            '.exe', '.dll', '.so', '.dylib',
            '.o', '.obj', '.lib', '.a'
        ]:
            continue
            
        total_files += 1
        if remove_emojis_from_file(filepath):
            changed_files.append(filepath)
    
    return changed_files, total_files

def main():
    """Main function."""
    if len(sys.argv) != 2:
        print("Usage: python3 remove_emojis.py <directory>")
        print("Example: python3 remove_emojis.py /path/to/project")
        return 1
    
    directory = sys.argv[1]
    
    if not os.path.isdir(directory):
        print(f"Error: Directory '{directory}' does not exist")
        return 1
    
    print(f" Removing emojis from: {directory}")
    
    changed_files, total_files = process_directory(directory)
    
    print(f"\n Summary:")
    print(f"  Total files processed: {total_files}")
    print(f"  Files modified: {len(changed_files)}")
    
    if changed_files:
        print(f"\n Modified files:")
        for filepath in changed_files:
            print(f"  - {filepath}")
    else:
        print("\n No emojis found in any files!")
    
    return 0

if __name__ == "__main__":
    sys.exit(main())
