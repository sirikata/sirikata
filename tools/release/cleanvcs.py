#!/usr/bin/env python


import sys, os
import subprocess

def remove_directory(d):
    for root, dirs, files in os.walk(d, topdown=False):
        for name in files:
            os.remove(os.path.join(root, name))
        for name in dirs:
            os.rmdir(os.path.join(root, name))
    os.rmdir(d)

def main():
    """Clears version control data so submodules and any other checked out data doesn't have unnecessary git/svn data stored with it for release."""
    base = sys.argv[1]

    for root, dirs, files in os.walk(base):
        if '.git' in dirs:
            dirs.remove('.git')
            remove_directory(os.path.join(root, '.git'))

if __name__ == "__main__":
    main()
