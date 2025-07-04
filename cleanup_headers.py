#!/usr/bin/env python3
"""
Remove unused header files from xserver repository based on strace analysis.
"""

import os
import sys

def cleanup_headers():
    """Remove header files not used during libxephyr build."""
    
    # Read list of used headers
    used_headers = set()
    with open("used_headers.txt", "r") as f:
        for line in f:
            header = line.strip()
            if header:
                used_headers.add(header)
    
    print(f"Found {len(used_headers)} used headers")
    
    # Find all header files in the project
    all_headers = []
    header_dirs = ["include", "Xext", "Xi", "config", "composite", "damageext", 
                  "dbe", "dri3", "exa", "fb", "glamor", "glx", "hw", "mi", 
                  "miext", "os", "present", "randr", "record", "render", 
                  "xfixes", "xkb", "dix", "test"]
    
    for dir_name in header_dirs:
        if os.path.exists(dir_name):
            for root, dirs, files in os.walk(dir_name):
                for file in files:
                    if file.endswith('.h'):
                        full_path = os.path.join(root, file)
                        # Convert to relative path from project root
                        rel_path = os.path.relpath(full_path, ".")
                        all_headers.append(rel_path)
    
    print(f"Found {len(all_headers)} total header files in project")
    
    # Find unused headers
    unused_headers = []
    for header in all_headers:
        if header not in used_headers:
            # Skip build directory headers
            if not header.startswith("build/"):
                unused_headers.append(header)
    
    print(f"Found {len(unused_headers)} unused headers to remove")
    
    # Remove unused headers
    removed_count = 0
    for header in sorted(unused_headers):
        if os.path.exists(header):
            print(f"Removing: {header}")
            os.remove(header)
            removed_count += 1
    
    print(f"\nRemoved {removed_count} unused header files")
    print(f"Kept {len(used_headers)} used header files")
    
    return 0

if __name__ == "__main__":
    sys.exit(cleanup_headers())