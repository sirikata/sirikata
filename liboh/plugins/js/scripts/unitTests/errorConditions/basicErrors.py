#!/usr/bin/python


from error import *


SegFaultError          = Error("Seg fault", lambda_checkForTextError("SIGSEV"));
BusError               = Error("Bus Error", lambda_checkForTextError("SIGBUS"));
AssertError            = Error("Assert Error",lambda_checkForTextError("SIGABRT"));
UnitTestFailError      = Error("Unit Test Fail Error", lambda_checkForTextError("UNIT_TEST_FAIL"));
ExceptionError         = Error("Emerson Exception Error", lambda_checkForTextError("Exception"));
TimedOutError          = Error("Unit Test Timed Out", lambda_checkForTextError("UNIT_TEST_TIMEOUT"));
