#!/usr/bin/python

from __future__ import print_function
import sys

import framework.tests.errors as TestErrors

class Test(object):

    class classproperty(property):
        def __get__(self, cls, owner):
            return self.fget.__get__(None, owner)()

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

    # The name of the test. It uses the name of the class by default,
    # so it usually isn't necessary to override this.
    _name = None
    @classproperty
    @classmethod
    def name(cls):
        if not cls._name:
            return cls.__module__ + '.' + cls.__name__
        return cls._name

    # Use this to express order dependencies for tests. For example,
    # if TestSuperBar uses feature Bar, you could have after =
    # [TestBar] to make sure TestBar runs before TestSuperBar.
    after = []

    # List of features touched by this test, giving an indication of
    # what parts of the system might have broken. Not functional in
    # any way, just reported upon failure.
    touches = []

    # Error conditions to check upon completion of the test. The
    # default set checks that the test exited cleanly (no timeout), no
    # exceptions, and no unit test failures indicated by scripts.
    errors = DefaultErrorConditions

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
        for s in self.errors:
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
