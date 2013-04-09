#!/usr/bin/env python

from __future__ import print_function
from framework.tests.world import WorldHttpTest
from httpcommand import OHObjectTest
import os.path, random, socket

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



class MultipleSS(object):
    '''
    Mixin to specify 2 space servers, 1 OH
    '''

    services = [
        {
            'name' : 'pinto',
            'type' : 'pinto',
            'args' : lambda x: x.pintoArgs(),
            'wait' : False
            },
        {
            'name' : 'space1',
            'type' : 'space',
            'args' : lambda x: x.spaceArgs(1),
            'wait' : False
            },
        {
            'name' : 'space2',
            'type' : 'space',
            'args' : lambda x: x.spaceArgs(2),
            'wait' : False
            },
        {
            'name' : 'oh',
            'type' : 'cppoh',
            'args' : lambda x: x.ohArgs(),
            'default' : True
            }
        ]

    pinto_port = random.randint(2000, 3000)

    layout = (2, 1, 1)
    region = ((-1000, -1000, -1000), (1000, 1000, 1000))

    oseg_prefix = 'foo'

    def pintoArgs(self):
        return {
            'port' : str(self.pinto_port),
            'handler' : 'rtreecut',
            }

    def spaceArgs(self, ss):
        return {
            'id' : str(ss),

            'servermap' : 'tabular',
            'servermap-options' : '--filename=servermap.txt',

            'layout' : "<%d,%d,%d>" % (self.layout[0], self.layout[1], self.layout[2]),
            'region' : "<<%f,%f,%f>,<%f,%f,%f>>" % (self.region[0][0], self.region[0][1], self.region[0][2], self.region[1][0], self.region[1][1], self.region[1][2]),

            'oseg' : 'redis',
            # Note we turn of transactions in redis for compatibility,
            # at the cost of potentially leaking some entries
            'oseg-options' : '--prefix=' + self.oseg_prefix + ' --transactions=false',

            'pinto' : 'master',
            'pinto-options' : '--host=' + str(socket.gethostname()) + ' --port=' + str(self.pinto_port)
            }

    def ohArgs(self):
        return {
            'servermap' : 'tabular',
            'servermap-options' : '--filename=servermap.txt',
            'object-factory-opts': '--db=not-a-real-db.db', # Disable loading objects from db
            'mainspace' : '12345678-1111-1111-1111-DEFA01759ACE'
            }

    def testPre(self):
        # Generate servermap
        port_base = 30000
        nservers = len([x for x in self.services if x['type'] == 'space'])
        hostname = socket.gethostname()
        with open('servermap.txt', 'w') as f:
            for ss in range(nservers):
                internal_port = port_base + 2*ss
                external_port = internal_port + 1
                print(hostname + ':' + str(internal_port) + ':' + str(external_port), file=f)

class MultipleManualSS(MultipleSS):
    '''
    Mixin to specify 1 space server using manual querying, 1 OH
    '''

    def pintoArgs(self):
        return dict(MultipleSS.pintoArgs(self).items() + ({'type' : 'manual'}).items())

    def spaceArgs(self, ss):
        return dict(MultipleSS.spaceArgs(self, ss).items() + ({'prox' : 'libprox-manual', 'pinto' : 'master-manual', 'moduleloglevel' : 'prox=detailed'}).items())

    def ohArgs(self):
        return dict(MultipleSS.ohArgs(self).items() + ({'oh.query-processor' : 'manual', 'moduleloglevel' : 'manual-query-processor=detailed'}).items())






class ConnectionTest(ProximityTest):
    after = [ OHObjectTest ]

    def testBody(self):
        response = self.createObject('oh', 'proximityTests/connectionTest.em');

class OneSSConnectionTest(ConnectionTest, OneSS):
    pass

class OneSSManualConnectionTest(ConnectionTest, OneManualSS):
    pass

class MultipleSSConnectionTest(ConnectionTest, MultipleSS):
    pass

class MultipleSSManualConnectionTest(ConnectionTest, MultipleManualSS):
    pass




class OIDRepeatConnectTest(ProximityTest):
    after = [ OHObjectTest ]

    def testBody(self):
        response = self.createObject('oh', 'proximityTests/oidRepeatConnect.em');

class OneSSOIDRepeatConnectTestKnownToFail(OIDRepeatConnectTest, OneSS):
    pass

#class OneSSOIDRepeatConnectTestKnownToFail(OIDRepeatConnectTest, OneManualSS):
#    pass

#class MultipleSSOIDRepeatConnectTestKnownToFail(OIDRepeatConnectTest, MultipleSS):
#    pass

#class MultipleSSManualOIDRepeatConnectTestKnownToFail(OIDRepeatConnectTest, MultipleManualSS):
#    pass




class BasicQueryTest(ProximityTest):
    after = [ OHObjectTest ]

    def testBody(self):
        response = self.createObject('oh', 'proximityTests/basicQueryTest.em');

class OneSSBasicQueryTest(BasicQueryTest, OneSS):
    after = [OneSSConnectionTest]

class OneSSManualBasicQueryTest(BasicQueryTest, OneManualSS):
    after = [OneSSManualConnectionTest]

class MultipleSSBasicQueryTest(BasicQueryTest, MultipleSS):
    after = [MultipleSSConnectionTest]

class MultipleSSManualBasicQueryTest(BasicQueryTest, MultipleManualSS):
    after = [MultipleSSManualConnectionTest]



class MigrationQueryTest(ProximityTest):
    after = [ OHObjectTest ]

    def testBody(self):
        response = self.createObject('oh', 'proximityTests/migrationQueryTest.em');

class MultipleSSMigrationQueryTest(MigrationQueryTest, MultipleSS):
    after = [MultipleSSConnectionTest]

class MultipleSSManualMigrationQueryTest(MigrationQueryTest, MultipleManualSS):
    after = [MultipleSSManualConnectionTest]
