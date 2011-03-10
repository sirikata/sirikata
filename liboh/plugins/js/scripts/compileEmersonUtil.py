#!/usr/bin/python

import subprocess
import sys
import os

#the path + name of the emerson binary.
EMERSON_BINARY_NAME = '../../../../build/cmake/emerson';


#takes in a foldername and returns the name of all files within that folder
def generateAllFilenamesToRun(folderName):
    results = [];
    for root, dirs, files in os.walk(folderName):
        results.extend( [os.path.join(root, name) for name in files] );
    return results;

#returns true if the emerson compiler gets through the file with no error.
#returns false if encounter a seg fault.
def runIndividualFile(filename):
    filer = open('/dev/null','rw');
    retVal = subprocess.call(EMERSON_BINARY_NAME + ' ' + filename,shell=True,stdout=filer,stderr=filer);
    filer.close();

    print(" This is retVal: "+ str(retVal));
    return (retVal == 0);
                    
                    

def runFilesInFolder(folderName):
    print("\n\nRunning all files in folder " + folderName + " through Emerson compiler.");
    
    allFilesToRun = generateAllFilenamesToRun(folderName);

    for s in allFilesToRun:
        passed = runIndividualFile(s);
        if (passed):
            print("Running: " + s + "..........pased");
        else:
            print("Running: " + s + "..........FAILED");

            


if __name__ == "__main__":
    if (len(sys.argv) != 2):
        print("Error: usage requires an argument specifying directory to run all files through emerson parser.");
    else:
        runFilesInFolder(sys.argv[1]);
    
