#!/usr/bin/python

import sys;

from pyTests.tee import Tee

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
    

    #sandboxTest: tests onPresenceConnected, all sandbox and capabilities calls, timeout, killEntity
    sandboxTestInfo = CSVConstructorInfo(script_type="js",
                                         script_contents="system.import('unitTests/emTests/sandboxTest.em');");
    sandboxTest     = csvTest.CSVTest("sandboxTest",
                               touches=['onPresenceConnected', 'capabilities', 'sandbox', 'timeout', 'killEntity'],
                               entityConstructorInfo=[sandboxTestInfo],
                               duration=7);
    testArray.append(sandboxTest);

    #messagingTest: tests onPresenceConnected, message syntax, onPresenceConnected, createPresence,
    #                     system.presences, serialization and deserialization (for messages),
    #                     msg.makeReply.
    messagingTestInfo = CSVConstructorInfo(script_type="js",
                                         script_contents="system.import('unitTests/emTests/messagingTest.em');");
    messagingTest     = csvTest.CSVTest("messagingTest",
                                        touches=['onPresenceConnected', 'message syntax', 'createPresence',
                                                 'system.presences', 'serialization', 'deserialization','makeReply'],
                                        entityConstructorInfo=[messagingTestInfo],
                                        duration=15);
    testArray.append(messagingTest);


    csvFeatureObjectEntInfo = CSVConstructorInfo(
        script_type="js",
        script_contents="system.import('unitTests/emTests/featureObjectTest.em');");
    
    featureObjTest     = csvTest.CSVTest("featureObjectTest",
                                         touches=['onPresenceConnected', 'message syntax', 'featureObject',
                                                  'serialization', 'deserialization','makeReply'],
                                         #should load same script on two entities.
                                         entityConstructorInfo=[csvFeatureObjectEntInfo,csvFeatureObjectEntInfo],
                                         duration=17);
    testArray.append(featureObjTest);

    
    

    connectionLoadTestInfo = CSVConstructorInfo(script_type="js",
                                                script_contents="system.import('unitTests/emTests/connectionLoadTest.em');");
    connectionLoadTest     = csvTest.CSVTest("connectionLoadTest",
                                             touches=['onPresenceConnected', 'onPresenceDisconnected'],
                                             entityConstructorInfo=[connectionLoadTestInfo],
                                             duration=90);
    testArray.append(connectionLoadTest);
    

    global manager
    manager = testManager.TestManager();
    manager.addTestArray(testArray);

def runSome(testNames, output=sys.stdout, saveOutput=False):
    global manager
    manager.runSomeTests(testNames, output=output, saveOutput=saveOutput);

def runAll(output=sys.stdout, saveOutput=False):
    global manager
    manager.runAllTests(output=output, saveOutput=saveOutput);


if __name__ == "__main__":

    saveOutput = False;
    output = sys.stdout

    testsToRun = []
    for s in range (1, len(sys.argv)):
        if (sys.argv[s] == '--saveOutput'):
            saveOutput=True;
        elif sys.argv[s].startswith('--log='):
            output = Tee( open( sys.argv[s].split('=', 1)[1], 'w'), sys.stdout )
        else:
            testsToRun.append(sys.argv[s])

    registerTests()
    if len(testsToRun):
        runSome(testsToRun, output, saveOutput)
    else:
        runAll(output, saveOutput);
