#!/usr/bin/python

import sys;

import pyTests.testManager as testManager
import pyTests.csvTest as csvTest
from dbGen.csvConstructorInfo import *
import dbGen.basicGenerators as basicGenerators
import errorConditions.basicErrors as basicErrors

manager = None

def registerTests():

    testArray = [];

    '''
    This is where you add in you tests.
    '''

    #create basic test: single entity runs for 10 seconds and shuts
    #down.
    basicTest = csvTest.CSVTest("basicTest",
                                [basicGenerators.KillAfterTenSecondsEntity],
                                [basicErrors.SegFaultError,
                                 basicErrors.BusError,
                                 basicErrors.AssertError],
                                howLongToRunInSeconds=20
                                );

    testArray.append(basicTest);


    #timeoutTest: see documentation in unitTests/emTests/timeoutTest.em
    #Tests: timeout, self, onPresenceConnected, Date
    timeoutTestInfo = CSVConstructorInfo(script_type=stringWrap("js"),
                                         script_contents=stringWrap("system.import('unitTests/emTests/timeoutTest.em');"));
    timeoutTest = csvTest.CSVTest("timeoutTest",
                                  touches=['timeout',
                                           'self',
                                           'onPresenceConnected',
                                           'Date'],

                                  entityConstructorInfo=[timeoutTestInfo],

                                  errorConditions=[basicErrors.SegFaultError,
                                                   basicErrors.BusError,
                                                   basicErrors.AssertError,
                                                   basicErrors.UnitTestFailError],

                                  howLongToRunInSeconds=100);

    testArray.append(timeoutTest);



    ##httpTest: see documentation in unitTests/emTests/httpTest.em
    httpTestInfo = CSVConstructorInfo(script_type=stringWrap("js"),
                                      script_contents=stringWrap("system.import('unitTests/emTests/httpTest.em');"));
    httpTest = csvTest.CSVTest("httpTest",
                                  touches=['http',
                                           'timeout'
                                           ],

                                  entityConstructorInfo=[httpTestInfo],

                                  errorConditions=[basicErrors.SegFaultError,
                                                   basicErrors.BusError,
                                                   basicErrors.AssertError,
                                                   basicErrors.UnitTestFailError],

                                  howLongToRunInSeconds=30);

    testArray.append(httpTest);



    #proximityAdded test: see documentation in unitTests/emTests/proximityAdded.em.
    #Tests: onProxAdded, setQueryAngle, setVelocity, createPresence,
    #       getProxSet, timeout, onPresenceConnected
    otherEntInfo = CSVConstructorInfo(
        pos_x=numWrap(10),
        pos_y=numWrap(0),
        pos_z=numWrap(0),
        scale=numWrap(1),
        solid_angle=numWrap(100)
        );

    csvProxAddedEntInfo = CSVConstructorInfo(
        script_type=stringWrap("js"),
        script_contents=stringWrap("system.import('unitTests/emTests/proximityAdded.em');"),
        pos_x=numWrap(0),
        pos_y=numWrap(0),
        pos_z=numWrap(0),
        scale=numWrap(1),
        solid_angle=numWrap(100)
        );

    proximityAddedTest = csvTest.CSVTest("proximityAdded",

                                         touches=['onProxAdded',
                                                  'setQueryAngle',
                                                  'setVelocity',
                                                  'createPresence',
                                                  'getProxSet',
                                                  'timeout',
                                                  'onPresenceConnected'],

                                         entityConstructorInfo=[otherEntInfo,
                                                                csvProxAddedEntInfo],

                                         errorConditions=[basicErrors.SegFaultError,
                                                          basicErrors.BusError,
                                                          basicErrors.AssertError,
                                                          basicErrors.UnitTestFailError],

                                         howLongToRunInSeconds=50
                                         );

    testArray.append(proximityAddedTest);


    ##createPresenceTest
    csvCreatePresEntInfo = CSVConstructorInfo(
        script_type=stringWrap("js"),
        script_contents=stringWrap("system.import('unitTests/emTests/createPresenceTest.em');"),
        pos_x=numWrap(20),
        pos_y=numWrap(0),
        pos_z=numWrap(0),
        scale=numWrap(1)
        );

    createPresenceTest = csvTest.CSVTest("createPresence",

                                         touches=['createPresence',
                                                  'onCreatePresence',
                                                  'vector and quat syntax',
                                                  'timeout',
                                                  'system.presences'
                                                  ],

                                         entityConstructorInfo=[csvCreatePresEntInfo],

                                         errorConditions=[basicErrors.SegFaultError,
                                                          basicErrors.BusError,
                                                          basicErrors.AssertError,
                                                          basicErrors.UnitTestFailError],

                                         howLongToRunInSeconds=50
                                         );

    testArray.append(createPresenceTest);

    global manager
    manager = testManager.TestManager();
    manager.addTestArray(testArray);

def runSome(testNames,saveOutput=False):
    global manager
    manager.runSomeTests(testNames, saveOutput=saveOutput);

def runAll(saveOutput=False):
    global manager
    manager.runAllTests(saveOutput=saveOutput);


if __name__ == "__main__":

    saveOutput = False;

    testsToRun = []
    for s in range (1, len(sys.argv)):
        if (sys.argv[s] == '--saveOutput'):
            saveOutput=True;
        else:
            testsToRun.append(sys.argv[s])

    registerTests()
    if len(testsToRun):
        runSome(testsToRun, saveOutput)
    else:
        runAll(saveOutput);
