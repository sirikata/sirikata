#!/usr/bin/python

from __future__ import print_function
import sys

import framework.tests.errors as TestErrors
import traceback # For default message to give some info about where the error originated

class Test(object):

    class classproperty(property):
        def __get__(self, cls, owner):
            return self.fget.__get__(None, owner)()

    DefaultErrorConditions = [
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

    # You can set disabled to True to remove a test from the active
    # test suite
    disabled = False

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


    def __init__(self):
        self._failed = False
        self._output = None

    def setOutput(self, output):
        self._output = output

    @property
    def failed(self):
        return self._failed


    def report(self):
        '''
        Report extra log information at the end of a run. By default,
        nothing is recorded here, but tests can override this to
        present additional information to the user after a failure.
        '''
        pass

    def fail(self, msg=None):
        self._failed = True
        if msg:
            print('FAILED:', msg, file=self._output)
        else:
            print('FAILED:', 'Test failed with traceback:', file=self._output)
            # Get stack trace, which comes back as list of strings for
            # each stack frame. We chop off the last one to get rid of
            # this frame in fail()
            print(''.join(traceback.format_stack()[:-1]), file=self._output)

    def assertTrue(self, expr, msg=None):
        if not expr: self.fail(msg=msg)

    def assertFalse(self, expr, msg=None):
        if expr: self.fail(msg=msg)

    def assertEqual(self, lhs, rhs, msg=None):
        if not lhs == rhs: self.fail(msg=msg)

    def assertNotEqual(self, lhs, rhs, msg=None):
        if lhs == rhs: self.fail(msg=msg)

    def assertIsNone(self, expr, msg=None):
        if expr is not None: self.fail(msg=msg)

    def assertIsNotNone(self, expr, msg=None):
        if expr is None: self.fail(msg=msg)

    def assertIsIn(self, val, col, msg=None):
        try:
            if val not in col: self.fail(msg=msg)
        except:
            self.fail(msg=msg)

    def assertIsNotIn(self, val, col, msg=None):
        try:
            if val in col: self.fail(msg=msg)
        except:
            self.fail(msg=msg)

    def assertEmpty(self, col, msg=None):
        try:
            if col is None or len(col) > 0: self.fail(msg=msg)
        except:
            self.fail(msg=msg)

    def assertNotEmpty(self, col, msg=None):
        try:
            if col is None or len(col) == 0: self.fail(msg=msg)
        except:
            self.fail(msg=msg)

    def assertLen(self, col, sz, msg=None):
        try:
            if col is None or len(col) != sz: self.fail(msg=msg)
        except:
            self.fail(msg=msg)

    def assertReturnCode(self, code):
        if code < 0:
            returnCodeErrorName = 'Unknown'
            if (code == -6):
                returnCodeErrorName = 'Assert fault';
            elif(code == -9):
                returnCodeErrorName = 'Killed';
            elif(code == -11):
                returnCodeErrorName = 'Seg fault';
            self.fail(msg='Negative return code: ' + returnCodeErrorName + '(' + str(code) + ')')

    def assertLogErrorFree(self, filename):
        '''
        @param {String} filename name of a file that contains the
        output of a run of the system.  Run through file applying error
        conditions as we go.
        '''

        fullFile = open(filename,'r').read();

        results = [];
        for s in self.errors:
            errorReturner = s.performErrorCheck(fullFile);
            results.append([s.getName(), errorReturner]);
            if errorReturner.getErrorExists():
                self.fail(s.getName() + ': ' + errorReturner.getErrorMessage())
