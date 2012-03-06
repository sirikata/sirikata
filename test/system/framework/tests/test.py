#!/usr/bin/python

from __future__ import print_function
import sys

import framework.tests.errors as TestErrors

class Test(object):
    DefaultErrorConditions = [
        TestErrors.ExceptionError,
        TestErrors.TimedOutError,
        TestErrors.UnitTestFailError
        ]
    # For tests that expect to timeout because they don't have simple
    # ending criteria.
    TimeOutTestErrorConditions = [
        TestErrors.ExceptionError,
        TestErrors.UnitTestFailError
        ]
    DefaultDuration = 20

    # Use this to express order dependencies for tests. For example,
    # if TestSuperBar uses feature Bar, you could have after =
    # [TestBar] to make sure TestBar runs before TestSuperBar.
    after = []

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
    @param {String} filenameToAnalyze name of a file that contains the
    output of a run of the system.  Run through file applying error
    conditions as we go.

    @param {Int} returnCode the code that the process running the test
    returned with.  On unix systems, indicates seg faults, bus errors,
    etc.  On windows, I don't think that this does anything.
    '''
    def analyzeOutput(self, filenameToAnalyze, returnCode, output=sys.stdout):
        fullFile = open(filenameToAnalyze,'r').read();

        failed = returnCode < 0;
        results = [];
        for s in self.errorConditions:
            errorReturner = s.performErrorCheck(fullFile);
            results.append([s.getName(), errorReturner]);
            failed = (failed or errorReturner.getErrorExists());

        if failed:
            print("TEST FAILED", file=output)
            print("  Features used by test: ", ', '.join(self.touches), file=output)
        else:
            print("TEST PASSED", file=output)


        for s in results:
            if (s[1].getErrorExists()):
                print(s[0] + ":", s[1].getErrorExists(), file=output)

        if returnCode < 0:
            returnCodeErrorName = 'Unknown'
            if (returnCode == -6):
                returnCodeErrorName = 'Assert fault';
            elif(returnCode == -9):
                returnCodeErrorName = 'Killed';
            elif(returnCode == -11):
                returnCodeErrorName = 'Seg fault';

            print("Error exit code:", returnCodeErrorName, "(" + str(returnCode) + ")", file=output)

        return not failed
