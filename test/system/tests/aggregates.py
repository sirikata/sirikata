#!/usr/bin/env python

from __future__ import print_function
from framework.tests.world import WorldHttpTest
import os, random, time

class AggregatesTest(WorldHttpTest):
    duration = 30
    wait_until_responsive = True
    script_paths = [ os.path.dirname(__file__) ]

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
        return {
            'servermap-options' : '--port=' + str(self.port),
            'prox.object.handler' : 'rtreecutagg',
            # This flag makes all uploads appear to be successful without a CDN
            'aggmgr.skip-upload' : 'true'
            }

    def ohArgs(self):
        return {
            'servermap-options' : '--port=' + str(self.port),
            'object-factory-opts': '--db=not-a-real-db.db', # Disable loading objects from db
            'mainspace' : '12345678-1111-1111-1111-DEFA01759ACE'
            }

    def num_aggregates(self):
        result = self.command('space', 'space.prox.handlers')
        self.assertIsNotNone(result)
        if not result: return 0
        self.assertIsIn('handlers', result)
        if 'handlers' not in result: return 0
        self.assertIsIn('object', result['handlers'])
        if 'object' not in result['handlers']: return 0
        handlers = result['handlers']['object']
        self.assertIsIn('static', handlers)
        self.assertIsIn('dynamic', handlers)
        if 'static' not in handlers or 'dynamic' not in handlers: return 0
        self.assertIsIn('nodes', handlers['static'])
        self.assertIsIn('objects', handlers['static'])
        self.assertIsIn('nodes', handlers['dynamic'])
        self.assertIsIn('objects', handlers['dynamic'])
        if 'nodes' not in handlers['static'] or 'objects' not in handlers['static'] or \
                'nodes' not in handlers['dynamic'] or 'objects' not in handlers['dynamic']:
            return 0
        # Num aggregates = num nodes - num leaf objects
        return (handlers['static']['nodes'] + handlers['static']['objects']) - (handlers['static']['objects'] + handlers['static']['objects'])

    def generated_aggregates(self):
        result = self.command('space', 'space.aggregates.stats')
        self.assertIsNotNone(result)
        if not result: return 0
        self.assertIsIn('stats', result)
        if 'stats' not in result: return 0
        self.assertIsIn('generated', result['stats'])
        if 'generated' not in result['stats']: return 0
        return result['stats']['generated']

    def shutdown(self):
        self.assertIsNotNone( self.command('oh', 'context.shutdown') )
        self.assertIsNotNone( self.command('space', 'context.shutdown') )

class GeneratesAggregatesTest(AggregatesTest):
    def testBody(self):
        # Generate a bunch of objects
        for x in range(25):
            response = self.createObject('oh', 'aggregationTests/object.em')

        timeout = 10.0
        num_agg = self.num_aggregates()
        gen_agg = self.generated_aggregates()
        while timeout > 0:
            if num_agg > 0 and gen_agg > num_agg: break
            time.sleep(0.25)
            timeout -= 0.25
            num_agg = self.num_aggregates()
            gen_agg = self.generated_aggregates()
        # Once we're out of time or verified, it's safe to make the assertion
        self.assertNotEqual(num_agg, 0, "Didn't generate any aggregates")
        self.assertTrue(gen_agg >= num_agg, "Only generated %d aggregates, at least %d expected" % (gen_agg, num_agg))

        self.shutdown()
