#!/usr/bin/env python

import sys, os
import subprocess

def is_bin(f):
    if sys.platform == 'linux2':
        file_type = subprocess.Popen(['file', '-Lb', f], stdout=subprocess.PIPE).communicate()[0]
        return file_type.startswith("ELF")


def strip(f):
    if sys.platform == 'linux2':
        subprocess.Popen(['strip', f]).communicate()

def main():
    """Strips symbols from all libraries and binaries. Takes one argument, the directory of the installation."""
    base = sys.argv[1]

    for root, dirs, files in os.walk(base):
        for name in files:
            fpath = os.path.join(root, name)
            if is_bin(fpath):
                strip(fpath)

if __name__ == "__main__":
    main()
