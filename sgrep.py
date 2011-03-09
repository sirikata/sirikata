#!/usr/bin/python

import subprocess
import sys

EXCLUDES = [ 'dependencies', 'externals', 'scripts', 'build/cmake', 'build/Frameworks', '.git', 'doc', 'docs' ]

if __name__ == "__main__":
    if (len (sys.argv) < 2):
        print("\n\nIncorrect usage: add in what you're grepping for\n\n")

    relative_excludes = [ './' + ex for ex in EXCLUDES ]
    EXCLUDES.extend(relative_excludes)

    #cmd = ['grep','-R', sys.argv[1]]
    cmd = ['grep','-R']
    cmd += [ '--exclude-dir=' + e for e in EXCLUDES ]
    cmd += sys.argv[1:]
    cmd += [ '.' ]

    subprocess.call(cmd);
