import os
import re
import sys

# Configuration
ROOT_DIR = os.getcwd()
ENGINE_DIR = os.path.join(ROOT_DIR, 'src/engine')
INCLUDE_DIRS = [
    os.path.join(ROOT_DIR, 'src/engine/include'),
    os.path.join(ROOT_DIR, 'src/engine'),
    os.path.join(ROOT_DIR, 'include')
]

# Extensions to scan
SRC_EXTENSIONS = ('.c', '.cpp', '.h', '.m', '.mm')
HDR_EXTENSIONS = ('.h', '.hpp')

def build_header_map():
    header_map = {}
    
    # helper to add paths
    def index_dir(base_path, strip_prefix):
        for root, dirs, files in os.walk(base_path):
            for file in files:
                if file.endswith(HDR_EXTENSIONS):
                    full_path = os.path.join(root, file)
                    rel_path = os.path.relpath(full_path, strip_prefix)
                    
                    if file not in header_map:
                        header_map[file] = []
                    header_map[file].append(rel_path)
    
    # Index from include directories
    for inc_dir in INCLUDE_DIRS:
        if os.path.exists(inc_dir):
            index_dir(inc_dir, inc_dir)
            
    return header_map

def fix_includes(header_map):
    modified_count = 0
    warning_count = 0
    
    for root, dirs, files in os.walk(ENGINE_DIR):
        for file in files:
            if file.endswith(SRC_EXTENSIONS):
                file_path = os.path.join(root, file)
                
                with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                    lines = f.readlines()
                
                new_lines = []
                file_modified = False
                
                for line in lines:
                    match = re.match(r'^\s*#include\s+["<]([^">]+)[">]', line)
                    if match:
                        old_path = match.group(1)
                        filename = os.path.basename(old_path)
                        
                        # Heuristic: Check if old path is still valid
                        is_valid = False
                        for inc_dir in INCLUDE_DIRS:
                            if os.path.exists(os.path.join(inc_dir, old_path)):
                                is_valid = True
                                break
                        
                        if not is_valid:
                            # Try to find new path
                            if filename in header_map:
                                candidates = sorted(list(set(header_map[filename])))
                                
                                # Heuristic 0: If one candidate starts with 'include/' and fits well, prefer it?
                                # Actually, let's just prioritize 'include/' slightly in the scoring
                                
                                if len(candidates) == 1:
                                    # Perfect match
                                    new_inc = candidates[0]
                                    if new_inc != old_path:
                                        line = line.replace(old_path, new_inc)
                                        file_modified = True
                                elif len(candidates) > 1:
                                    # Ambiguous
                                    # Heuristic: Prefer candidate that shares the most directory prefix with old_path
                                    best_candidate = None
                                    best_common_len = -1
                                    
                                    old_dir = os.path.dirname(old_path)
                                    # Also consider the current file's directory to resolve common local includes
                                    current_file_dir_rel = os.path.relpath(os.path.dirname(file_path), ENGINE_DIR)

                                    for cand in candidates:
                                        cand_dir = os.path.dirname(cand)
                                        
                                        # Score 1: Match against old include path (migrated folder)
                                        # e.g. old: rendering/renderer.h, new: rendering/core/renderer.h
                                        common_p = os.path.commonprefix([old_dir, cand_dir])
                                        score = len(common_p) * 2
                                        
                                        # Score 2: Match against current file's locations (sibling)
                                        common_local = os.path.commonprefix([current_file_dir_rel, cand_dir])
                                        if len(common_local) > 0: 
                                            score += len(common_local) * 3  # Strong boost for siblings

                                        # Score 3: Prefer include/ for stability if no strong sibling match
                                        if cand.startswith('include/'):
                                            score += 1

                                        if score > best_common_len:
                                            best_common_len = score
                                            best_candidate = cand
                                    
                                    if best_candidate and best_common_len > 0:
                                        new_inc = best_candidate
                                        if new_inc != old_path:
                                            line = line.replace(old_path, new_inc)
                                            file_modified = True
                                    else:
                                        warning_count += 1
                                        with open('ambiguous_includes.log', 'a') as log:
                                            log.write(f"{file}: {old_path} -> {candidates}\n")
                                else:
                                    # Should not happen given the check above
                                    pass
                            else:
                                # Header not found anywhere
                                # print(f"Warning: Header not found: {old_path} in {file}")
                                pass
                    
                    new_lines.append(line)
                
                if file_modified:
                    with open(file_path, 'w', encoding='utf-8') as f:
                        f.writelines(new_lines)
                    modified_count += 1
                    
    print(f"Fixed includes in {modified_count} files.")
    print(f"Encountered {warning_count} ambiguous cases.")

if __name__ == "__main__":
    print("Building header map...")
    h_map = build_header_map()
    print(f"Indexed {len(h_map)} unique header names.")
    print("Fixing includes...")
    fix_includes(h_map)
