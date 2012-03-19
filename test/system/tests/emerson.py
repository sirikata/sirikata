#!/usr/bin/python

from framework.tests.csv import CSVTest
from framework.db.entity import Entity
import os

class EmersonFeatureTest(CSVTest):
    script_paths = [ os.path.join(os.path.dirname(__file__), 'emTests') ]


class BasicTest(EmersonFeatureTest):
    '''Most basic test with Emerson, creating an object and then shutting it down immediately.'''
    duration = 25
    entities = [Entity(script_type="js",
                       script_contents="system.import('basicPresence.em');")
                ]

class UserEventTest(EmersonFeatureTest):
    after = [BasicTest]
    entities = [Entity(script_type="js",
                       script_contents="system.import('userEventTest.em');")
                ]
    touches=['system.event']

class TimeoutTest(EmersonFeatureTest):
    after = [BasicTest]
    entities = [Entity(script_type="js",
                       script_contents="system.import('timeoutTest.em');")
                ]
    touches = ['timeout', 'self', 'onPresenceConnected', 'Date']
    duration = 105

class HttpTest(EmersonFeatureTest):
    after = [BasicTest]
    entities = [Entity(script_type="js",
                       script_contents="system.import('httpTest.em');")
                ]
    touches = ['http', 'timeout']
    duration = 35

class CreatePresenceTest(EmersonFeatureTest):
    after = [TimeoutTest]
    entities = [Entity(script_type="js",
                       script_contents="system.import('createPresenceTest.em');",
                       pos_x=20, pos_y=0, pos_z=0)
                ]
    touches=['createPresence', 'onCreatePresence', 'vector and quat syntax', 'timeout', 'system.presences'],
    duration = 55

class ProximityAddedTest(EmersonFeatureTest):
    after = [TimeoutTest, CreatePresenceTest]

    entities = [
        Entity(pos_x=10, pos_y=0, pos_z=0, solid_angle=100),
        Entity(
            script_type="js",
            script_contents="system.import('proximityAdded.em');",
            pos_x=0, pos_y=0, pos_z=0, solid_angle=100
            )
        ]

    touches=['onProxAdded', 'setQueryAngle', 'setVelocity',
             'createPresence', 'getProxSet', 'timeout', 'onPresenceConnected']
    duration = 30
    needs_hup  = True

class PresenceEventsTest(EmersonFeatureTest):
    after = [TimeoutTest]
    entities = [ Entity(script_type="js",
                        script_contents="system.import('presenceEventsTest.em');")
                 ]
    touches = ['system.createPresence', 'system.onPresenceConnected', 'system.onPresenceDisconnected']
    duration = 15

class SerializationTest(EmersonFeatureTest):
    #serializationTest: tests objects, arrays, and functions serialized.  does not test certain special objects (presences, visibles, etc.).
    after = [BasicTest]
    entities = [Entity(script_type="js",
                       script_contents="system.import('serializationTest.em');")
                ]
    touches = ['system.onPresenceConnected', 'serialize','deserialize','disconnect']
    duration = 15

class StorageTest(EmersonFeatureTest):
    after = [TimeoutTest, SerializationTest]
    entities = [Entity(script_type="js",
                       script_contents="system.import('storageTest.em');")
                ]
    touches = ['storageBeginTransaction', 'storageCommit', 'storageErase', 'storageRead', 'storageWrite', 'serialization', 'timeout', 'killEntity']
    duration = 15

class SandboxTest(EmersonFeatureTest):
    after = [PresenceEventsTest]
    entities = [Entity(script_type="js",
                       script_contents="system.import('sandboxTest.em');")
                ]
    touches = ['onPresenceConnected', 'capabilities', 'sandbox', 'timeout', 'killEntity']
    duration = 15

class MessagingTest(EmersonFeatureTest):
    after = [PresenceEventsTest]
    entities = [Entity(script_type="js",
                       script_contents="system.import('messagingTest.em');")
                ]
    touches = ['onPresenceConnected', 'message syntax', 'createPresence',
               'system.presences', 'serialization', 'deserialization','makeReply']
    duration = 20

class SelfTest(EmersonFeatureTest):
    after = [TimeoutTest, HttpTest, StorageTest, PresenceEventsTest, MessagingTest]
    entities = [Entity(script_type="js",
                       script_contents="system.import('selfTest.em');")
                ]
    touches = ['system.self']
    duration = 65

class CSVFeatureTest(EmersonFeatureTest):
    after = [PresenceEventsTest]
    disabled = True # Currently fails and not required

    # should load same script on two entities.
    _eci = Entity(script_type="js",
                  script_contents="system.import('featureObjectTest.em');")
    entities = [_eci, _eci]
    touches=['onPresenceConnected', 'message syntax', 'featureObject',
             'serialization', 'deserialization','makeReply']
    duration = 25

class ConnectionLoadTest(EmersonFeatureTest):
    after = [PresenceEventsTest]
    entities = [Entity(script_type="js",
                       script_contents="system.import('connectionLoadTest.em');")
                ]
    touches = ['onPresenceConnected', 'onPresenceDisconnected']
    duration = 95
