#!/usr/bin/python


from error import *


UnitTestFailError      = Error("Unit Test Fail Error", lambda_checkForTextError("UNIT_TEST_FAIL"));
ExceptionError         = Error("Emerson Exception Error", lambda_checkForTextError("Exception"));
TimedOutError          = Error("Unit Test Timed Out", lambda_checkForTextError("UNIT_TEST_TIMEOUT"));
