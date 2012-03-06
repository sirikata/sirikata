#!/usr/bin/python

from framework.pyTests.csvTest import CSVTest
from framework.dbGen.csvConstructorInfo import CSVConstructorInfo
import framework.dbGen.basicGenerators as basicGenerators
import framework.errorConditions.basicErrors as basicErrors
import os

class EmersonFeatureTest(CSVTest):
    script_paths = [ os.path.join(os.path.dirname(__file__), 'emTests') ]


class BasicTest(EmersonFeatureTest):
    #create basic test: single entity runs for 10 seconds and shuts
    #down.
    def __init__(self):
        super(BasicTest, self).__init__("basicTest",
                                        [basicGenerators.KillAfterTenSecondsEntity],
                                        duration=25
                                        )

class TimeoutTest(EmersonFeatureTest):
    #timeoutTest: see documentation in unitTests/emTests/timeoutTest.em
    #Tests: timeout, self, onPresenceConnected, Date
    def __init__(self):
        timeoutTestInfo = CSVConstructorInfo(script_type="js",
                                             script_contents="system.import('timeoutTest.em');");
        super(TimeoutTest, self).__init__("timeoutTest",
                                          touches=['timeout',
                                                   'self',
                                                   'onPresenceConnected',
                                                   'Date'],
                                          entityConstructorInfo=[timeoutTestInfo],
                                          duration=105)


class HttpTest(EmersonFeatureTest):
    ##httpTest: see documentation in unitTests/emTests/httpTest.em
    def __init__(self):
        httpTestInfo = CSVConstructorInfo(script_type="js",
                                          script_contents="system.import('httpTest.em');");
        super(HttpTest, self).__init__("httpTest",
                                       touches=['http',
                                                'timeout'
                                                ],
                                       entityConstructorInfo=[httpTestInfo],
                                       duration=35);

class ProximityAddedTest(EmersonFeatureTest):
    #proximityAdded test: see documentation in unitTests/emTests/proximityAdded.em.
    #Tests: onProxAdded, setQueryAngle, setVelocity, createPresence,
    #       getProxSet, timeout, onPresenceConnected
    def __init__(self):
        otherEntInfo = CSVConstructorInfo(pos_x=10, pos_y=0, pos_z=0, solid_angle=100);

        csvProxAddedEntInfo = CSVConstructorInfo(
            script_type="js",
            script_contents="system.import('proximityAdded.em');",
            pos_x=0, pos_y=0, pos_z=0, solid_angle=100
            )
        super(ProximityAddedTest, self).__init__("proximityAdded",
                                                 touches=['onProxAdded',
                                                          'setQueryAngle',
                                                          'setVelocity',
                                                          'createPresence',
                                                          'getProxSet',
                                                          'timeout',
                                                          'onPresenceConnected'],
                                                 entityConstructorInfo=[otherEntInfo,
                                                                        csvProxAddedEntInfo],
                                                 errorConditions = EmersonFeatureTest.TimeOutTestErrorConditions,
                                                 duration=30
                                                 )

class CreatePresenceTest(EmersonFeatureTest):
    ##createPresenceTest
    def __init__(self):
        csvCreatePresEntInfo = CSVConstructorInfo(
            script_type="js",
            script_contents="system.import('createPresenceTest.em');",
            pos_x=20, pos_y=0, pos_z=0
            )

        super(CreatePresenceTest, self).__init__("createPresence",
                                                 touches=['createPresence',
                                                          'onCreatePresence',
                                                          'vector and quat syntax',
                                                          'timeout',
                                                          'system.presences'
                                                          ],
                                                 entityConstructorInfo=[csvCreatePresEntInfo],
                                                 duration=55
                                                 )

class UserEventTest(EmersonFeatureTest):
    #userEventTest: tests system.event functionality
    def __init__(self):
        userEventTestInfo = CSVConstructorInfo(script_type="js",
                                               script_contents="system.import('userEventTest.em');");
        super(UserEventTest, self).__init__("userEventTest",
                                            touches=['system.event'],
                                            entityConstructorInfo=[userEventTestInfo]);


class SelfTest(EmersonFeatureTest):
    #selfTest: tests system.self value/restoration after various operations
    def __init__(self):
        selfTestInfo = CSVConstructorInfo(script_type="js",
                                          script_contents="system.import('selfTest.em');");
        super(SelfTest, self).__init__("selfTest",
                                       touches=['system.self'],
                                       entityConstructorInfo=[selfTestInfo],
                                       duration=65)

class PresenceEventsTest(EmersonFeatureTest):
    #presenceEventsTest: tests events triggered on presences' connection and disconnection
    def __init__(self):
        presenceEventsTestInfo = CSVConstructorInfo(script_type="js",
                                                    script_contents="system.import('presenceEventsTest.em');");
        super(PresenceEventsTest, self).__init__("presenceEventsTest",
                                                  touches=['system.createPresence', 'system.onPresenceConnected', 'system.onPresenceDisconnected'],
                                                  entityConstructorInfo=[presenceEventsTestInfo],
                                                  duration=15)

class SerializationTest(EmersonFeatureTest):
    #serializationTest: tests objects, arrays, and functions serialized.  does not test certain special objects (presences, visibles, etc.).
    def __init__(self):
        serializationTestInfo = CSVConstructorInfo(script_type="js",
                                                   script_contents="system.import('serializationTest.em');");
        super(SerializationTest, self).__init__("serializationTest",
                                                touches=['system.onPresenceConnected', 'serialize','deserialize','disconnect'],
                                                entityConstructorInfo=[serializationTestInfo],
                                                duration=15)

class StorageTest(EmersonFeatureTest):
    #storageTest: tests system.storageBeginTransaction, system.storageCommit, system.storageErase, system.storageRead, system.storageWrite, serialization, timeout,killEntity
    def __init__(self):
        storageTestInfo = CSVConstructorInfo(script_type="js",
                                             script_contents="system.import('storageTest.em');");
        super(StorageTest, self).__init__("storageTest",
                                          touches=['storageBeginTransaction', 'storageCommit', 'storageErase', 'storageRead', 'storageWrite', 'serialization', 'timeout', 'killEntity'],
                                          entityConstructorInfo=[storageTestInfo],
                                          duration=15)

class SandboxTest(EmersonFeatureTest):
    #sandboxTest: tests onPresenceConnected, all sandbox and capabilities calls, timeout, killEntity
    def __init__(self):
        sandboxTestInfo = CSVConstructorInfo(script_type="js",
                                             script_contents="system.import('sandboxTest.em');");
        super(SandboxTest, self).__init__("sandboxTest",
                                          touches=['onPresenceConnected', 'capabilities', 'sandbox', 'timeout', 'killEntity'],
                                          entityConstructorInfo=[sandboxTestInfo],
                                          duration=15)

class MessagingTest(EmersonFeatureTest):
    #messagingTest: tests onPresenceConnected, message syntax, onPresenceConnected, createPresence,
    #                     system.presences, serialization and deserialization (for messages),
    #                     msg.makeReply.
    def __init__(self):
        messagingTestInfo = CSVConstructorInfo(script_type="js",
                                               script_contents="system.import('messagingTest.em');");
        super(MessagingTest, self).__init__("messagingTest",
                                            touches=['onPresenceConnected', 'message syntax', 'createPresence',
                                                     'system.presences', 'serialization', 'deserialization','makeReply'],
                                            entityConstructorInfo=[messagingTestInfo],
                                            duration=20)

class CSVFeatureTest(EmersonFeatureTest):

    def __init__(self):
        csvFeatureObjectEntInfo = CSVConstructorInfo(
            script_type="js",
            script_contents="system.import('featureObjectTest.em');");

        super(CSVFeatureTest, self).__init__("featureObjectTest",
                                             touches=['onPresenceConnected', 'message syntax', 'featureObject',
                                                      'serialization', 'deserialization','makeReply'],
                                             #should load same script on two entities.
                                             entityConstructorInfo=[csvFeatureObjectEntInfo,csvFeatureObjectEntInfo],
                                             duration=25)

class ConnectionLoadTest(EmersonFeatureTest):
    def __init__(self):
        connectionLoadTestInfo = CSVConstructorInfo(script_type="js",
                                                    script_contents="system.import('connectionLoadTest.em');");
        super(ConnectionLoadTest, self).__init__("connectionLoadTest",
                                                 touches=['onPresenceConnected', 'onPresenceDisconnected'],
                                                 entityConstructorInfo=[connectionLoadTestInfo],
                                                 duration=95)
