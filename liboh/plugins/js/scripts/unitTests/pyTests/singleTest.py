#!/usr/bin/python



class SingleTest:
    
    '''
    
    @param {String} testName identifies the name of the test that
    we're running.
    
    @param {Array} (optional) errorConditions Each element of the
    errorConditions array should be a CheckError class.  After running
    the test, we run each of the error condition checks agains the
    output of the test.  If any of the error conditions return true,
    then we report that the test has failed.  Otherwise, we report a
    success.

    @param {String} additionalCMDLineArgs Specify any additional
    arguments you want to provide to cpp_oh.  Note do not specify
    object-factory or object-factory-opts (they're being used by this class).
    '''
    
    def __init__(self,testName, errorConditions=[], additionalCMDLineArgs=''):
        self.testName = testName;
        self.errorConditions = errorConditions;
        self.additionalCMDLineArgs = '';

    '''
    @see errorConditions param of __init__
    '''
    def addErrorCondition(self,errToAdd):
        self.errorConditions.append(errToAdd);

    def addArrayOfErrorConditions(self,arrayErrToAdd):
        for s in arrayErrToAdd:
            self.addErrorCondition(s);

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
        outputFile.write('**************************\n');
        outputFile.write('Performing test for ' + self.testName + '\n');
        

        failed = False;
        results = [];
        for s in self.errorConditions:
            errorReturner = s.performErrorCheck(fullFile);
            results.append([s.getName(), errorReturner]);
            failed = (failed or errorReturner.getErrorExists());

        outputFile.write('result:  ');
        if (failed):
            outputFile.write('FAILED');
        else:
            outputFile.write('PASSED');

        outputFile.write('\n\nDetailed information: \n');
        for s in results:
            outputFile.write('\t' + s[0] + ':    ');
            if (s[1].getErrorExists()):
                outputFile.write(' FAILED\n');
                outputFile.write('\t\tinfo: ' + s[1].getErrorMessage());
            else:
                outputFile.write(' PASSED');

            outputFile.write('\n');
            

        outputFile.flush();
        outputFile.close();
        
