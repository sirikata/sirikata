#!/usr/bin/python

import sys
import subprocess
import os.path

jsdocDir = "./jsdoc-toolkit"

def getJSDocForSphinx():
    if not os.path.exists('jsdoc-toolkit/templates/jsdoc-for-sphinx'):
        subprocess.Popen(['git',
                          'clone',
                          'git://github.com/jterrace/jsdoc-for-sphinx.git',
                          'jsdoc-toolkit/templates/jsdoc-for-sphinx']).communicate()

def getJSDoc():
    if not os.path.exists('jsdoc-toolkit'):
        subprocess.Popen(['svn', 'checkout', 'http://jsdoc-toolkit.googlecode.com/svn/trunk/jsdoc-toolkit', 'jsdoc-toolkit']).communicate()

def runJSDoc(emroot, outFold, templateDir):
    libroot = emroot + '/scripts/std/'
    cmd = 'bash %s/jsrun.sh -ap -t=%s -u -x=em -r=10 -d=%s %s' % (jsdocDir, templateDir, outFold, libroot)
    envir = {}
    envir["JSDOCDIR"] = jsdocDir
    subprocess.Popen(cmd.split(), env=envir).communicate()[0]


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
    templateDir = jsdocDir + '/templates/jsdoc'
    for s in range(1,len(sys.argv)):
        if (sys.argv[s] == '-outFold'):
            if (s+1 < len(sys.argv)):
                outFold = sys.argv[s+1]

        if (sys.argv[s] == '-emroot'):
          if (s + 1 < len(sys.argv)):
            emroot = sys.argv[s + 1]
            
        if (sys.argv[s] == '--sphinx'):
            templateDir = jsdocDir + '/templates/jsdoc-for-sphinx'

    if ((outFold == None) or (emroot == None)):
        print("Usage error.  Specify emerson root folder (-emroot) and output folder (-outFold)")
    else:
        emroot = os.path.abspath(emroot)
        outFold = os.path.abspath(outFold)
        getJSDoc()
        getJSDocForSphinx()
        runJSDoc(emroot, outFold, templateDir)
