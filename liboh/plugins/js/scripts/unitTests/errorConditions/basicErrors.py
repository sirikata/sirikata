#!/usr/bin/python


from error import *


SegFaultError          = Error("Seg fault", lambda_checkForTextError("SIGSEV"));
BusError               = Error("Bus Error", lambda_checkForTextError("SIGBUS"));
AssertError            = Error("Assert Error",lambda_checkForTextError("SIGABRT"));
UnitTestFailError      = Error("Unit Test Fail Error", lambda_checkForTextError("UNIT_TEST_FAIL"));
UnitTestNoSuccessError = Error("Unit Test No Success Error", lambda_checkForNotInTextError("UNIT_TEST_SUCCESS"));
ExceptionError         = Error("Emerson Exception Error", lambda_checkForTextError("Exception"));


