#!/usr/bin/python

import subprocess
import os

REGRESSION_FOLDER                    =                               "tests/";
TESTS_REGRESSION_FOLDER              =     REGRESSION_FOLDER + "testEmerson/";
CORRECT_REGRESSION_FOLDER            =            REGRESSION_FOLDER + "refs/";
CORRECT_REGRESSION_SUFFIX            =                                 ".out";
EMERSON_UTIL_FOLDER                  =           REGRESSION_FOLDER + "utils/";
EMERSON_COMPILED_FILENAME            =                "__EMERSON_compiled.js";
EMERSON_COMPILED_OUTPUT              =        "__EMERSON_compiledRhinoed.txt";
EMERSON_TMP_DIFF_RESULT_FILENAME     =            "__EMERSON_diffResults.txt";
EMERSON_BINARY_NAME                  =   "../../../../../build/cmake/emerson";
REGRESSION_RESULTS_FILENAME          =                "regressionResults.txt";
REGRESSION_EMERSON_LIBRARY_FILENAME  =                    "emersonLibrary.em";

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
def regressionPass(filenameToRegress, fileFolderToRegress, correctFolder, debug=False):

    #compiles filenameToRegress to EMERSON_COMPILED_FILENAME
    compileSucceeded = emersonCompile(fileFolderToRegress+'/'+filenameToRegress,EMERSON_COMPILED_FILENAME);

    if (not compileSucceeded):
        diffResult = "COMPILE_ERROR"
        return RegressionResults.COMPILE_ERROR, diffResult

    #runs EMERSON_COMPILED_FILENAME through rhino
    runRhino(EMERSON_COMPILED_FILENAME,EMERSON_COMPILED_OUTPUT);


    if (not debug):
        #cleans up the EMERSON_COMPILED_FILENAME
        subprocess.call('rm ' + EMERSON_COMPILED_FILENAME, shell=True);


    #run diff
    result, diffResult = runDiff(EMERSON_COMPILED_OUTPUT,correctFolder + '/'+filenameToRegress + CORRECT_REGRESSION_SUFFIX);



    if (not debug):
        #cleans up the EMERSON_COMPILED_OUTPUT
        subprocess.call('rm ' + EMERSON_COMPILED_OUTPUT, shell=True);

    return result, diffResult


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
    #subprocess.call('rhino -f ' + toRunOn, stdout=filer,stderr=filer,shell=True);
    subprocess.call('rhino -f ' + REGRESSION_EMERSON_LIBRARY_FILENAME + " " + toRunOn, stdout=filer,stderr=filer,shell=True);
    filer.flush();
    filer.close();


'''
filenameDiff1 and filename Diff2 are the names of the files to
diff against each other.

diffResult is a list.  We append the result of the diff to the end of it.
'''
def runDiff(filenameDiff1, filenameDiff2):
    #hack-ish.  piping the output of the diff command to a file, then re-reading the
    #file all at once, appending it to diffResult, and deleting file
    filer = open(EMERSON_TMP_DIFF_RESULT_FILENAME,'w');
    subprocess.call('diff ' + filenameDiff1 + ' ' + filenameDiff2, stdout=filer, stderr=filer, shell=True);
    filer.flush();
    filer.close();

    #read the file into a string that we append to diffResult
    filer    = open(EMERSON_TMP_DIFF_RESULT_FILENAME,'r');
    allDiffs = filer.read();

    diffResult = allDiffs
    filer.close();

    #remove the temporary file that the diff was saved to
    subprocess.call('rm ' + EMERSON_TMP_DIFF_RESULT_FILENAME, shell=True);

    if (len(allDiffs) == 0):
        return RegressionResults.REGRESSION_SUCCEEDED, diffResult

    return RegressionResults.DIFF_FAILED, diffResult



'''
Goes into folder specified by TESTS_REGRESSION_FOLDER, runs regressions on each file
in that folder, and pretty-prints summary results as well as saving more in-depth results
(that include diffs) to REGRESSION_RESULTS_FILENAME
'''
def runAllRegressions():
    filenamesToRegress = gatherFilenames(TESTS_REGRESSION_FOLDER);

    resultFile = open (REGRESSION_RESULTS_FILENAME,'w')

    for s in filenamesToRegress:
        regOutcome, diffResult = regressionPass(s,TESTS_REGRESSION_FOLDER,CORRECT_REGRESSION_FOLDER);
        printResult(regOutcome,diffResult,s,resultFile);

    resultFile.flush();
    resultFile.close();

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
def printResult(regResult,diffResult,associatedFilename,fileToPrintDetailsTo):
    fileToPrintDetailsTo.write("\n\n*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*\n");
    msg = "";

    if (regResult == RegressionResults.COMPILE_ERROR):
        msg = associatedFilename + " ........... FAILED: compile error";
    elif (regResult == RegressionResults.DIFF_FAILED):
        msg = associatedFilename + " ........... FAILED: diff failed";
    elif (regResult == RegressionResults.REGRESSION_SUCCEEDED):
        msg = associatedFilename + " ........... passed";
    else:
        print("\n\nUnknown regression result: " + str(regResult) + " for filename " + associatedFilename + ".  Aborting\n\n");
        assert False;

    print(msg);
    fileToPrintDetailsTo.write(msg);
    fileToPrintDetailsTo.write("\n\n");
    fileToPrintDetailsTo.write("Diff results:\n");
    fileToPrintDetailsTo.write(diffResult);
    fileToPrintDetailsTo.write("\n\n");


if __name__ == "__main__":
    runAllRegressions();
