#!/usr/bin/env python

from __future__ import print_function
from framework.tests.world import WorldHttpTest
from httpcommand import OHObjectTest
import os.path, random

class ProximityTest(WorldHttpTest):
    script_paths = [ os.path.dirname(__file__) ]

class OneSS(object):
    '''
    Mixin to specify 1 space server, 1 OH
    '''

    services = [
        {
            'name' : 'space',
            'type' : 'space',
            'args' : lambda x: x.spaceArgs(),
            'wait' : False
            },
        {
            'name' : 'oh',
            'type' : 'cppoh',
            'args' : lambda x: x.ohArgs(),
            'default' : True
            }
        ]

    # Random port to avoid conflicts
    port = random.randint(2000, 3000)

    def spaceArgs(self):
        return { 'servermap-options' : '--port=' + str(self.port) }

    def ohArgs(self):
        return {
            'servermap-options' : '--port=' + str(self.port),
            'object-factory-opts': '--db=not-a-real-db.db', # Disable loading objects from db
            'mainspace' : '12345678-1111-1111-1111-DEFA01759ACE'
            }

class OneManualSS(OneSS):
    '''
    Mixin to specify 1 space server using manual querying, 1 OH
    '''

    def spaceArgs(self):
        return dict(OneSS.spaceArgs(self).items() + ({'prox' : 'libprox-manual', 'moduleloglevel' : 'prox=detailed'}).items())

    def ohArgs(self):
        return dict(OneSS.ohArgs(self).items() + ({'oh.query-processor' : 'manual', 'moduleloglevel' : 'manual-query-processor=detailed'}).items())



class ConnectionTest(ProximityTest):
    after = [ OHObjectTest ]

    def testBody(self):
        response = self.createObject('oh', 'proximityTests/connectionTest.em');

class OneSSConnectionTest(ConnectionTest, OneSS):
    pass

class OneSSManualConnectionTest(ConnectionTest, OneManualSS):
    pass



class BasicQueryTest(ProximityTest):
    after = [ OHObjectTest ]

    def testBody(self):
        response = self.createObject('oh', 'proximityTests/basicQueryTest.em');

class OneSSBasicQueryTest(BasicQueryTest, OneSS):
    after = [OneSSConnectionTest]

class OneSSManualBasicQueryTest(BasicQueryTest, OneManualSS):
    after = [OneSSConnectionTest]
