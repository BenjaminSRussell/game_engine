#!/usr/bin/env python3
"""
Comprehensive Audit Script (Phases 1-6)
Parses phase*_prompts.md files to find expected files and checks their existence.
"""

import os
import re
from pathlib import Path

BASE_DIR = "/Users/benjaminrussell/Desktop/Minecraft v2/src/engine/rendering/3d_rendering"
AGENTS_DIR = os.path.join(BASE_DIR, "agents")

def parse_phase_file(filepath):
    """Extracts expected files and target directories from a phase prompt file."""
    with open(filepath, 'r') as f:
        content = f.read()

    agents = []
    # Split by "## Agent" sections
    sections = re.split(r'^## Agent', content, flags=re.MULTILINE)
    
    for section in sections[1:]: # Skip preamble
        # Extract Directories
        dirs_match = re.search(r'DIRECTORIES TO WORK IN:(.*?)YOUR RESPONSIBILITIES', section, re.DOTALL)
        target_dirs = []
        if dirs_match:
            dir_block = dirs_match.group(1)
            # listed as "- src/..."
            paths = re.findall(r'-\s+src/engine/rendering/3d_rendering/([^\s]+)', dir_block)
            target_dirs = [p.strip() for p in paths]

        # Extract Files
        files = []
        # Look for numbered list items ending in .c
        # e.g. "1. device_context.c - Initialize..."
        file_matches = re.findall(r'^\d+\.\s+([\w\.]+\.c)', section, re.MULTILINE)
        files = file_matches

        if files:
            agents.append({
                "dirs": target_dirs,
                "files": files
            })
    
    return agents

def find_file(filename, search_dirs, base_dir):
    # 1. Check specific expected directories
    for d in search_dirs:
        path = os.path.join(base_dir, d, filename)
        if os.path.exists(path):
            return path, "FOUND"
            
    # 2. Check anywhere in base_dir
    for root, _, files in os.walk(base_dir):
        if filename in files:
            return os.path.join(root, filename), "MISPLACED"
            
    return None, "MISSING"

print("="*60)
print("PHASE 3-6 MISSING FILE AUDIT")
print("="*60)

missing_count = 0
misplaced_count = 0

# Check Phases 3 to 6
for i in range(3, 7):
    prompt_file = os.path.join(AGENTS_DIR, f"phase{i}_prompts.md")
    if not os.path.exists(prompt_file):
        print(f"Skipping Phase {i} (File not found: {prompt_file})")
        continue
        
    print(f"\nAnalyzing Phase {i}...")
    agents = parse_phase_file(prompt_file)
    
    for agent_idx, agent in enumerate(agents):
        # We assume agent order matches 3.1, 3.2 etc
        agent_name = f"Agent {i}.{agent_idx + 1}"
        
        for filename in agent['files']:
            path, status = find_file(filename, agent['dirs'], BASE_DIR)
            
            if status == "MISSING":
                print(f"  [MISSING] {agent_name}: {filename}")
                print(f"    Expected in: {', '.join(agent['dirs'])}")
                missing_count += 1
            elif status == "MISPLACED":
                # checking if it's a subfolder of an expected dir (which counts as found for our purposes usually)
                is_sub = False
                rel_path = os.path.relpath(path, BASE_DIR)
                for d in agent['dirs']:
                    if rel_path.startswith(d):
                        is_sub = True
                        break
                
                if not is_sub:
                    print(f"  [MISPLACED] {agent_name}: {filename}")
                    print(f"    Found at: {rel_path}")
                    misplaced_count += 1

print("-" * 60)
print(f"Total Missing: {missing_count}")
print(f"Total Misplaced: {misplaced_count}")
