#!/usr/bin/python

import subprocess
import sys
import os.path

# These are generic rules that should only match on the individual
# directory name (rather than the entire path, e.g. bar in
# /foo/bar). These should be used rarely since they can end up
# accidentally block other directories (e.g. putting in 'scripts'
# would block out both the top-level scripts directory and Emerson
# scripts).
RELATIVE_EXCLUDES = [
    '.git'
]

# These are 'absolute' paths, w.r.t. the top Sirikata directory
ABSOLUTE_EXCLUDES = [
    'dependencies',
    'externals',
    'scripts',
    'build/cmake',
    'build/Frameworks',
    'doc',
    'docs',
    'liboh/plugins/ogre/data/ace',
    'liboh/plugins/ogre/data/labjs',
    'liboh/plugins/ogre/data/jquery_themes',
    'liboh/plugins/ogre/data/jquery_plugins',
    'liboh/plugins/ogre/data/jquery',
    'liboh/plugins/js/emerson/alt_regress',
    'liboh/plugins/js/emerson/regression',
    'libproxyobject/plugins/ogre/data/ace/build/src'
    ]

if __name__ == "__main__":
    if (len (sys.argv) < 2):
        print("Usage: sgrep.py pattern")


    sirikata_dir = os.path.abspath(os.path.dirname(__file__))

    excludes = []
    excludes += RELATIVE_EXCLUDES
    excludes += [os.path.join(sirikata_dir, p) for p in ABSOLUTE_EXCLUDES]

    cmd = ['grep', '-R']
    cmd += [ '--exclude-dir=' + e for e in excludes ]
    cmd += sys.argv[1:]
    cmd += [ sirikata_dir ]

    subprocess.call(cmd);
