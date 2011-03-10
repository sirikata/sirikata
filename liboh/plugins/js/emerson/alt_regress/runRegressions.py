#!/usr/bin/python

import subprocess
import os

REGRESSION_FOLDER                    =                               "tests/";
TESTS_REGRESSION_FOLDER              =     REGRESSION_FOLDER + "testEmerson/";
CORRECT_REGRESSION_FOLDER            =            REGRESSION_FOLDER + "refs/";
EMERSON_UTIL_FOLDER                  =           REGRESSION_FOLDER + "utils/";
EMERSON_COMPILED_FILENAME            =                "__EMERSON_compiled.js";
EMERSON_COMPILED_OUTPUT              =        "__EMERSON_compiledRhinoed.txt";
EMERSON_TMP_DIFF_RESULT_FILENAME     =            "__EMERSON_diffResults.txt";
EMERSON_BINARY_NAME                  =   "../../../../../build/cmake/emerson";
REGRESSION_RESULTS_FILENAME          =                "regressionResults.txt";


class RegressionResults:
    COMPILE_ERROR        = 1;
    DIFF_FAILED          = 2;
    REGRESSION_SUCCEEDED = 3;
    



'''
Must have rhino installed on machine running regressions on.
'''



'''
Runs the file fileFolderToRegress/filenameToRegress through the Emerson
compiler and outputs to EMERSON_COMPILED_FILENAME.
Runs this file through rhino, and outputs both stdout and stderr
to EMERSON_COMPILED_OUTPUT.

Then checks EMERSON_COMPILED_OUTPUT against similarly named file in correctFolder.
Ie.  diffs EMERSON_COMPILED_OUTPUT with correctFolder/filenameToRegress.

The results of the diff are stored in the diffResult list.

If debug is false, then cleans up the intermediat emerson_compiled_filename

Returns RegressionResult value 
'''
def regressionPass(filenameToRegress, fileFolderToRegress, correctFolder, diffResult, debug=False):
    
    #compiles filenameToRegress to EMERSON_COMPILED_FILENAME
    compileSucceeded = emersonCompile(fileFolderToRegress+'/'+filenameToRegress,EMERSON_COMPILED_FILENAME);
    if (not compileSucceeded):
        diffResult.append("COMPILE_ERROR");
        return RegressionResults.COMPILE_ERROR;

    #runs EMERSON_COMPILED_FILENAME through rhino
    runRhino(EMERSON_COMPILED_FILENAME,EMERSON_COMPILED_OUTPUT);

    
    if (not debug):
        #cleans up the EMERSON_COMPILED_FILENAME
        subprocess.call('rm ' + EMERSON_COMPILED_FILENAME, shell=True);


    #run diff
    result = runDiff(EMERSON_COMPILED_OUTPUT,correctFolder + '/'+filenameToRegress,diffResult);


    
    if (not debug):
        #cleans up the EMERSON_COMPILED_OUTPUT
        subprocess.call('rm ' + EMERSON_COMPILED_OUTPUT, shell=True);
     
    return result;
        

'''
Requests Emerson compiler to compile the file with filename toCompile and output its results to
toCompileTo.
'''
def emersonCompile(toCompile, toCompileTo):
    filer = open(toCompileTo,'w');
    compRes = subprocess.call(EMERSON_BINARY_NAME + ' -f ' + toCompile,shell=True,stdout=filer,stderr=filer);
    filer.flush();
    filer.close();

    if (compRes == 0):
        #compilation exited without failure.
        return True;

    #compilation threw an error.
    return False;




'''
toRunOn is the name of the file to pass in to rhino.
toOutputTo is the filename for the file that we pass stderr and stdout from the rhino process to.
'''
def runRhino (toRunOn, toOutputTo):
    filer = open(toOutputTo,'w');
    subprocess.call('rhino -f ' + toRunOn, stdout=filer,stderr=filer,shell=True);
    filer.flush();
    filer.close();


'''
filenameDiff1 and filename Diff2 are the names of the files to
diff against each other.

diffResult is a list.  We append the result of the diff to the end of it.
'''
def runDiff(filenameDiff1, filenameDiff2,diffResult):
    #hack-ish.  piping the output of the diff command to a file, then re-reading the
    #file all at once, appending it to diffResult, and deleting file
    filer = open(EMERSON_TMP_DIFF_RESULT_FILENAME,'w');
    subprocess.call('diff ' + filenameDiff1 + ' ' + filenameDiff2, stdout=filer, stderr=filer, shell=True);
    filer.flush();
    filer.close();

    #read the file into a string that we append to diffResult
    filer    = open(EMERSON_TMP_DIFF_RESULT_FILENAME,'r');
    allDiffs = filer.read();
    
    diffResult.append(allDiffs);
    filer.close();

    #remove the temporary file that the diff was saved to
    subprocess.call('rm ' + EMERSON_TMP_DIFF_RESULT_FILENAME, shell=True);
    
    if (len(allDiffs) == 0):
        return RegressionResults.REGRESSION_SUCCEEDED;

    return RegressionResults.DIFF_FAILED;



'''
Goes into folder specified by TESTS_REGRESSION_FOLDER, runs regressions on each file
in that folder, and pretty-prints summary results as well as saving more in-depth results
(that include diffs) to REGRESSION_RESULTS_FILENAME
'''
def runAllRegressions():
    filenamesToRegress = gatherFilenames(TESTS_REGRESSION_FOLDER);

    regResults  = [];
    diffResults = [];
    for s in filenamesToRegress:
        regOutcome = regressionPass(s,TESTS_REGRESSION_FOLDER,CORRECT_REGRESSION_FOLDER,diffResults);
        regResults.append(regOutcome);

    printAllResults(regResults,diffResults,filenamesToRegress,REGRESSION_RESULTS_FILENAME);

'''
Returns all files in folderToGatherFrom that have a suffix .em.
Does *not* return full pathname to file, just the filename within that
directory.
'''
def gatherFilenames(folderToGatherFrom):
    allFiles = os.listdir(folderToGatherFrom);
    return [x for x in allFiles if x.endswith('.em')]
    

'''
regResults is a list of RegressionResults
diffResults is a list of strings that are the diffs associated with regression results
associatedFilenames is a list of strings of the filenames of files that were regressed

Entries in regResults, diffResults, and associatedFilenames should all be consistent across
regressed files.  That is, regResults[2] should refer to the same regression file as diffResults[2],
which should refer to the same regression file as associatedFilenames[2].  (For this reason,
vectors should be of equal length.)

This function prints only high-level pass/fail information to stdout.  It saves more detailed
information about diffs to file named by filenameToPrintDetailsTo.
'''
def printAllResults(regResults,diffResults,associatedFilenames,filenameToPrintDetailsTo):
    if ((len(regResults) != len(diffResults)) or (len(regResults) != len(associatedFilenames))):
        print("can only print results over vectors of equal length");
        print(len(regResults));
        print(len(diffResults));
        print(len(associatedFilenames));
        assert False;

    filer = open (filenameToPrintDetailsTo,'w');
    for s in range (0, len(regResults)):
        filer.write("\n\n*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*\n");
        msg = "";
        
        if (regResults[s] == RegressionResults.COMPILE_ERROR):
            msg = associatedFilenames[s] + " ........... FAILED: compile error";
        elif (regResults[s] == RegressionResults.DIFF_FAILED):
            msg = associatedFilenames[s] + " ........... FAILED: diff failed";
        elif (regResults[s] == RegressionResults.REGRESSION_SUCCEEDED):
            msg = associatedFilenames[s] + " ........... passed";
        else:
            print("\n\nUnknown regression result: " + str(regResults[s]) + " for filename " + associatedFilenames[s] + ".  Aborting\n\n");
            assert False;

        print(msg);
        filer.write(msg);
        filer.write("\n\n");
        filer.write("Diff results:\n");
        filer.write(diffResults[s]);
        filer.write("\n\n");
            
    filer.flush();
    filer.close();


    
if __name__ == "__main__":
    runAllRegressions();
