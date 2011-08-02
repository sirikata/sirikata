#!/usr/bin/python

import errorConditions.basicErrors as basicErrors

class SingleTest:
    DefaultErrorConditions = [
        basicErrors.SegFaultError,
        basicErrors.BusError,
        basicErrors.AssertError,
        basicErrors.ExceptionError,
        basicErrors.TimedOutError
        ]
    DefaultDuration = 20

    '''
    
    @param {String} name identifies the name of the test that
    we're running.
    
    @param {Array} (optional) errorConditions Each element of the
    errorConditions array should be a CheckError class.  After running
    the test, we run each of the error condition checks agains the
    output of the test.  If any of the error conditions return true,
    then we report that the test has failed.  Otherwise, we report a
    success.

    @param {Array} additionalCMDLineArgs Specify any additional
    arguments you want to provide to cpp_oh.  Each element of the
    array should correspond to an arg.

    @param {Int} duration Number of seconds to run test simulation for

    @param {Array} touches An array of strings.  Each element
    indicates a potential call that could have caused problem if test failed.
    '''
    
    def __init__(self, name, errorConditions=DefaultErrorConditions, additionalCMDLineArgs=None, duration=DefaultDuration, touches=None):
        self.testName = name;

        if (errorConditions == None):
            self.errorConditions = [];
        else:
            self.errorConditions = errorConditions;

        if (additionalCMDLineArgs == None):
            self.additionalCMDLineArgs = [];
        else:
            self.additionalCMDLineArgs = additionalCMDLineArgs;

        
        self.duration = duration;

        if (touches == None):
            self.touches = [];
        else:
            self.touches = touches;


    '''
    Don't know how to make this function like a purely virtual
    function in c++.  Instead just going to assert false if it's ever
    run.  All superclasses of single test need to overwrite this
    function.
    '''
    def runTest(self):
        print('\n\nError in unit test code of ' + self.testName + '.  Purely virtual runTest function.  This IS NOT an error with the system.  This is an error in how we wrote one of the test functions.  Aborting.');
        assert(false);


    '''
    @param {String} filenameToAnalyze name of a file that contains the
    output of a run of the system.  Run through file applying error
    conditions as we go.
    
    @param {String} fNameAppendResTo filename that we should append the
    results of this test to.
    
    
    Appends success and failure information to file with name
    self.outFName.
    '''
    def analyzeOutput(self, filenameToAnalyze, fNameAppendResTo):
        fullFile = open(filenameToAnalyze,'r').read();

        outputFile = open(fNameAppendResTo,'a');
        stringToPrint = "";
        stringToPrint += '**************************\n'
        stringToPrint += 'Performing test for ' + self.testName + '\n';
        

        failed = False;
        results = [];
        for s in self.errorConditions:
            errorReturner = s.performErrorCheck(fullFile);
            results.append([s.getName(), errorReturner]);
            failed = (failed or errorReturner.getErrorExists());


        stringToPrint += 'result:  '
        if (failed):
            stringToPrint += 'FAILED';
            stringToPrint += '\nCalls made by test: \n';
            for s in self.touches:
                stringToPrint += s + ',';
        else:
            stringToPrint += 'PASSED';

        stringToPrint += '\n\nDetailed information: \n';
        for s in results:
            stringToPrint += '\t' + s[0] + ':    '
            if (s[1].getErrorExists()):
                stringToPrint += ' FAILED\n';
                stringToPrint += '\t\tinfo: ' + s[1].getErrorMessage();
            else:
                stringToPrint += ' PASSED';
                

            stringToPrint += '\n';
            

        outputFile.write(stringToPrint);
        print(stringToPrint);
        outputFile.flush();
        outputFile.close();
        
