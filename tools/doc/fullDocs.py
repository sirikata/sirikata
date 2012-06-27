#!/usr/bin/python

import sys
import subprocess
import os.path
import shutil

def _check_output(*popenargs, **kwargs):
    r"""Run command with arguments and return its output as a byte string.

    Backported from Python 2.7 as it's implemented as pure python on stdlib.

    >>> check_output(['/usr/bin/python', '--version'])
    Python 2.6.2
    """
    process = subprocess.Popen(stdout=subprocess.PIPE, *popenargs, **kwargs)
    output, unused_err = process.communicate()
    retcode = process.poll()
    if retcode:
        cmd = kwargs.get("args")
        if cmd is None:
            cmd = popenargs[0]
        error = subprocess.CalledProcessError(retcode, cmd)
        error.output = output
        raise error
    return output

try:
    subprocess.check_output
except AttributeError:
    subprocess.check_output = _check_output

CURDIR = os.path.dirname(os.path.abspath(__file__))

EMERSON_BUILDER = os.path.join(CURDIR, 'emersonDocs.py')
DOCS_DIR = os.path.join(CURDIR, 'sirikata-docs')
VERSION_TXT = os.path.join(DOCS_DIR, 'source', 'version.txt')
EM_TARGET_OUTPUT = os.path.join(DOCS_DIR, 'source', 'api')
HTML_BUILD_OUTPUT = os.path.join(DOCS_DIR, 'build', 'dirhtml')
VERSION_H = os.path.join(CURDIR, '..', '..', 'libcore', 'include', 'sirikata', 'core', 'util', 'Version.hpp')

def getSirikataDocs():
    if os.path.exists(DOCS_DIR):
        subprocess.Popen(['git',
                          'pull',
                          'origin',
                          'master'],
                         cwd=DOCS_DIR).communicate()
    else:
        subprocess.Popen(['git',
                          'clone',
                          'git://github.com/sirikata/sirikata-docs.git',
                          DOCS_DIR]).communicate()

def buildSirikataDocs():
    subprocess.Popen(['make',
                      'dirhtml'],
                     cwd=DOCS_DIR).communicate()

def buildEmersonSphinx(emroot):
    subprocess.Popen([EMERSON_BUILDER,
                      '-emroot',
                      emroot,
                      '-outFold',
                      EM_TARGET_OUTPUT,
                      '--sphinx'],
                     cwd=DOCS_DIR).communicate()

'''
  Must be run with two parameters:

    -outFold string: name of a directory where you want to put
     generated documentation files.  (note: if directory doesn't
     exist, we create it.)
     
    -emroot string: path to the Emerson cpp root directory.  For
     example, as of commit 853abe811, you should point to
     <path to local sirikata>/liboh/plugins/js/
'''
if __name__ == "__main__":
    outFold = None
    emroot = None
    for s in range(1,len(sys.argv)):
        if (sys.argv[s] == '-outFold'):
            if (s+1 < len(sys.argv)):
                outFold = sys.argv[s+1]

        if (sys.argv[s] == '-emroot'):
          if (s + 1 < len(sys.argv)):
            emroot = sys.argv[s + 1]

    if ((outFold == None) or (emroot == None)):
        sys.stderr.write("Usage error.  Specify emerson root folder (-emroot) and output folder (-outFold)\n")
        sys.exit(1)
        
    emroot = os.path.abspath(emroot)
    outFold = os.path.abspath(outFold)
    getSirikataDocs()
    buildEmersonSphinx(emroot)
    
    version_num = subprocess.check_output(['grep', 'SIRIKATA_VERSION ', VERSION_H])
    # --> #define SIRIKATA_VERSION "0.0.25"
    # --> "0.0.25"
    # --> 0.0.25
    version_num = version_num.strip().split()[2][1:-1]
    with open(VERSION_TXT, 'w') as f:
        f.write(version_num)
        f.write('\n')
    
    buildSirikataDocs()
    subprocess.Popen(['cp',
                      '-rv',
                      HTML_BUILD_OUTPUT,
                      outFold]).communicate()
    