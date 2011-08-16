#!/usr/bin/python

import sys;

import pyTests.testManager as testManager
import pyTests.csvTest as csvTest
from dbGen.csvConstructorInfo import CSVConstructorInfo
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
                                duration=20
                                );

    testArray.append(basicTest);


    #timeoutTest: see documentation in unitTests/emTests/timeoutTest.em
    #Tests: timeout, self, onPresenceConnected, Date
    timeoutTestInfo = CSVConstructorInfo(script_type="js",
                                         script_contents="system.import('unitTests/emTests/timeoutTest.em');");
    timeoutTest = csvTest.CSVTest("timeoutTest",
                                  touches=['timeout',
                                           'self',
                                           'onPresenceConnected',
                                           'Date'],
                                  entityConstructorInfo=[timeoutTestInfo],
                                  duration=100);

    testArray.append(timeoutTest);



    ##httpTest: see documentation in unitTests/emTests/httpTest.em
    httpTestInfo = CSVConstructorInfo(script_type="js",
                                      script_contents="system.import('unitTests/emTests/httpTest.em');");
    httpTest = csvTest.CSVTest("httpTest",
                                  touches=['http',
                                           'timeout'
                                           ],
                                  entityConstructorInfo=[httpTestInfo],
                                  duration=30);

    testArray.append(httpTest);



    #proximityAdded test: see documentation in unitTests/emTests/proximityAdded.em.
    #Tests: onProxAdded, setQueryAngle, setVelocity, createPresence,
    #       getProxSet, timeout, onPresenceConnected
    otherEntInfo = CSVConstructorInfo(pos_x=10, pos_y=0, pos_z=0, solid_angle=100);

    csvProxAddedEntInfo = CSVConstructorInfo(
        script_type="js",
        script_contents="system.import('unitTests/emTests/proximityAdded.em');",
        pos_x=0, pos_y=0, pos_z=0, solid_angle=100
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
                                         errorConditions = csvTest.CSVTest.TimeOutTestErrorConditions,
                                         duration=25
                                         );
    testArray.append(proximityAddedTest);


    ##createPresenceTest
    csvCreatePresEntInfo = CSVConstructorInfo(
        script_type="js",
        script_contents="system.import('unitTests/emTests/createPresenceTest.em');",
        pos_x=20, pos_y=0, pos_z=0
        );

    createPresenceTest = csvTest.CSVTest("createPresence",
                                         touches=['createPresence',
                                                  'onCreatePresence',
                                                  'vector and quat syntax',
                                                  'timeout',
                                                  'system.presences'
                                                  ],
                                         entityConstructorInfo=[csvCreatePresEntInfo],
                                         duration=50
                                         );
    testArray.append(createPresenceTest);

    #userEventTest: tests system.event functionality
    userEventTestInfo = CSVConstructorInfo(script_type="js",
                                           script_contents="system.import('unitTests/emTests/userEventTest.em');");
    userEventTest = csvTest.CSVTest("userEventTest",
                                  touches=['system.event'],
                                  entityConstructorInfo=[userEventTestInfo]);
    testArray.append(userEventTest);

    #selfTest: tests system.self value/restoration after various operations
    selfTestInfo = CSVConstructorInfo(script_type="js",
                                      script_contents="system.import('unitTests/emTests/selfTest.em');");
    selfTest = csvTest.CSVTest("selfTest",
                               touches=['system.self'],
                               entityConstructorInfo=[selfTestInfo],
                               duration=60);
    testArray.append(selfTest);

    #presenceEventsTest: tests events triggered on presences' connection and disconnection
    presenceEventsTestInfo = CSVConstructorInfo(script_type="js",
                                      script_contents="system.import('unitTests/emTests/presenceEventsTest.em');");
    presenceEventsTest = csvTest.CSVTest("presenceEventsTest",
                               touches=['system.createPresence', 'system.onPresenceConnected', 'system.onPresenceDisconnected'],
                               entityConstructorInfo=[presenceEventsTestInfo],
                               duration=10);
    testArray.append(presenceEventsTest);


    #serializationTest: tests objects, arrays, and functions serialized.  does not test certain special objects (presences, visibles, etc.).
    serializationTestInfo = CSVConstructorInfo(script_type="js",
                                               script_contents="system.import('unitTests/emTests/serializationTest.em');");
    serializationTest = csvTest.CSVTest("serializationTest",
                               touches=['system.onPresenceConnected', 'serialize','deserialize','disconnect'],
                               entityConstructorInfo=[serializationTestInfo],
                               duration=10);
    testArray.append(serializationTest);
    

    #storageTest: tests system.storageBeginTransaction, system.storageCommit, system.storageErase, system.storageRead, system.storageWrite, serialization, timeout,killEntity
    storageTestInfo = CSVConstructorInfo(script_type="js",
                                         script_contents="system.import('unitTests/emTests/storageTest.em');");
    storageTest     = csvTest.CSVTest("storageTest",
                               touches=['storageBeginTransaction', 'storageCommit', 'storageErase', 'storageRead', 'storageWrite', 'serialization', 'timeout', 'killEntity'],
                               entityConstructorInfo=[storageTestInfo],
                               duration=10);
    testArray.append(storageTest);
    
    

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
