#!/usr/bin/env python3
"""
Trace include files used during ninja build with directory context.
"""

import subprocess
import re
import os
import sys

def trace_build():
    """Run ninja build with strace and capture include files with directory context."""
    
    # Change to build directory
    build_dir = "build"
    if not os.path.exists(build_dir):
        print(f"Error: {build_dir} directory does not exist")
        return 1
    
    os.chdir(build_dir)
    
    print("Cleaning previous build...")
    subprocess.run(["ninja", "clean"], capture_output=True)
    
    print("Running strace on ninja build...")
    cmd = ["strace", "-f", "-e", "trace=openat,chdir", "ninja", "-j1"]
    
    # Run strace and capture output
    process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, 
                               universal_newlines=True, bufsize=1)
    
    current_dir = os.getcwd()  # Track current working directory
    include_files = set()
    
    try:
        # Write full strace to file
        with open("../full_strace.txt", "w") as f:
            for line in process.stderr:
                f.write(line)
                f.flush()
                
                # Parse chdir to track directory changes
                chdir_match = re.search(r'chdir\("([^"]+)"\)', line)
                if chdir_match:
                    new_dir = chdir_match.group(1)
                    if new_dir.startswith('/'):
                        current_dir = new_dir
                    else:
                        current_dir = os.path.join(current_dir, new_dir)
                    print(f"Directory changed to: {current_dir}")
                
                # Parse openat for .h files
                openat_match = re.search(r'openat\(AT_FDCWD, "([^"]+\.h)"', line)
                if openat_match:
                    header_path = openat_match.group(1)
                    
                    # Resolve relative paths
                    if header_path.startswith('../'):
                        # Relative to current directory
                        resolved_path = os.path.normpath(os.path.join(current_dir, header_path))
                    elif header_path.startswith('/'):
                        # Absolute path
                        resolved_path = header_path
                    else:
                        # Relative to current directory
                        resolved_path = os.path.normpath(os.path.join(current_dir, header_path))
                    
                    # Only collect project headers (not system headers)
                    if not resolved_path.startswith('/usr/') and not resolved_path.startswith('/opt/'):
                        include_files.add(resolved_path)
                        print(f"Found header: {resolved_path}")
    
    except KeyboardInterrupt:
        print("Interrupted by user")
        process.terminate()
        return 1
    
    process.wait()
    
    # Go back to parent directory
    os.chdir("..")
    
    # Write collected headers to file
    print(f"\nCollected {len(include_files)} unique project header files:")
    
    # Filter to project headers only
    project_headers = []
    project_root = os.getcwd()
    
    for header in sorted(include_files):
        if header.startswith(project_root) or header.startswith('../'):
            # Convert to relative path from project root
            if header.startswith(project_root):
                rel_path = os.path.relpath(header, project_root)
            else:
                rel_path = header
            project_headers.append(rel_path)
    
    # Write to file
    with open("used_headers.txt", "w") as f:
        for header in sorted(project_headers):
            f.write(f"{header}\n")
            print(f"  {header}")
    
    print(f"\nUsed headers written to used_headers.txt")
    return 0

if __name__ == "__main__":
    sys.exit(trace_build())