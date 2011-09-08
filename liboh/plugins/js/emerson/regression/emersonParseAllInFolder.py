#!/usr/bin/python

import sys;
import os;
import subprocess;

EMERSON_BINARY_PATH = '../../../../../build/cmake/emerson';

def isEmersonFile(filename):
    if (len(filename) < 3):
        return False;

    return (filename[-3:] == '.em');
    

def runAll(binaryName, folderName,filer):
    for dirpath,dirnames,filenames in  os.walk(folderName):
        #check all the emerson files in this folder
        toCheck = [ x for x in filenames if isEmersonFile(x)];
        for s in toCheck:
            filer.write('\n\n-----------------------------------\nRunning: ' + os.path.join(dirpath,s) + '\n');
            filer.flush();

            sbproc = subprocess.Popen([binaryName, '-f', os.path.join(dirpath,s)], stdout=filer, stderr=filer);
            sbproc.wait();
            filer.flush();
            
        #recursively check all directory names in this folder by re-running through runAll
        for s in dirnames:
            runAll(binaryName, os.path.join(folderName, s),filer);
        
        
if __name__ == "__main__":
    '''
    First argument folder to walk files through.  Note, will only walk files that end with .em.
    Second argument name of file to save output of all compiles to.
    '''
    if (len(sys.argv) != 3):
        print('Usage Error: requires name of folder from whence to walk files.');
    else:
        filer = open (str(sys.argv[2]),'w');
        runAll(EMERSON_BINARY_PATH, str(sys.argv[1]),filer);
        filer.flush();
        filer.close();
    
