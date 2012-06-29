#!/usr/bin/env python

from __future__ import print_function
from httpcommand import HttpCommandTest, OHObjectTest
import sys, os, random, subprocess, traceback, time
from framework.procset import ProcSet

class ProximityTest(HttpCommandTest):
    '''
    Run a system of space servers and object hosts to test proximity
    results. This class provides utilities for running the system,
    selecting the type of query system to use, and for adding and
    removing objects during tests.
    '''

    duration = 30
    wait_until_responsive = True
    script_paths = [ os.path.dirname(__file__) ]

    after = [ OHObjectTest ]

    # You must override these settings
    #services = [
        # e.g.:
        #{
        #    'name' : 'space1',
        #    'type' : 'space',
        #    'args' : { 'arg1' : 'val1' } # or callable returning dict
        #    }
    #]

    def args(self, svc):
        '''Override to add additional args computed at run
        time. Helpful if you can't put static args in the args field
        of a services entry'''
        return {}

    def runTest(self, outputPath, binPath, cppohBinName, spaceBinName, output=sys.stdout):
        bins = {
            'space' : os.path.join(binPath, spaceBinName),
            'cppoh' : os.path.join(binPath, cppohBinName)
            }

        procs = ProcSet()
        self.report_files = {}
        self._outputs = {}
        for svc_idx,svc in enumerate(self.services):
            svc_name = svc['name']

            service_output_filename = os.path.join(outputPath, svc_name + '.log')

            # Add with http port
            self.add_http(svc_name)
            service_cmd = [bins[svc['type']]]
            service_cmd += self.http_settings(svc_name)
            if svc['type'] == 'cppoh':
                service_cmd.append('--objecthost=--scriptManagers=js:{--import-paths=%s}' % (','.join(self.script_paths)))
            if 'args' in svc:
                args = svc['args']
                if callable(args): args = args(self)
                service_cmd += ['--' + k + '=' + v for k,v in args.iteritems()]

            service_output = open(service_output_filename, 'w')
            print(' '.join(service_cmd), file=service_output)
            service_output.flush()
            default_proc = (('default' in svc) and svc['default']) or False
            wait_proc = ('wait' not in svc) or svc['wait']
            procs.process(service_cmd, stdout=service_output, stderr=subprocess.STDOUT, default=default_proc, wait=wait_proc)

            self._outputs[svc_name] = service_output
            self.report_files[svc_name] = service_output_filename

            # If requested, block for upto 10 seconds for the process to start responding to commands
            if self.wait_until_responsive:
                result = self.command(svc_name, 'meta.commands', retries=40, wait=.25, output=output)
                if result is None: self.fail('Server never became responsive to HTTP commands')

        # Defer to the implementation for sending and checking test
        # commands. Catch all errors so we can make sure we cleanup the
        # processes properly and report crashes, etc.
        try:
            self.testBody(procs, output=output)
        except:
            self.fail(msg='Uncaught exception during test:\n' + traceback.format_exc())

        # The implementation should know whether things are shutting
        # down cleanly. We just make sure we clean up aggressively if
        # necessary.
        if not procs.done():
            procs.wait(until=self.duration, killAt=self.duration, output=output)

        # Print a notification if we had to kill this process
        if procs.killed():
            self.fail('Timed out')

        for output in self._outputs.values():
            output.close()
        for output_filename in self.report_files.values():
            self.assertLogErrorFree(service_output_filename)

        self.assertReturnCode(procs.returncode())

        #self.report()

    def report(self):
        print("Execution Log:", file=self._output)
        for report_name, report_file in self.report_files.iteritems():
            print("  ", report_name, file=self._output)
            fp = open(report_file, 'r')
            for line in fp.readlines():
                print("    ", line, end='', file=self._output)
            fp.close()
            print(file=self._output)


    def createObject(self, oh, script_file):
        '''
        Helper for tests that creates an object on the given OH. The
        script should be a filename (under proximityTests). Returns
        the ID (internal OH ID, not presence) of the created object.
        '''
        response = self.command(oh, 'oh.objects.create',
                                { 'script' :
                                  { 'type' : 'js',
                                    'contents': 'system.import("' + script_file + '");'
                                    }
                                  })
        self.assertIsNotNone(response)
        self.assertIsNotIn('error', response)
        self.assertIsIn('id', response)
        objid = response['id']
        self.assertNotEmpty(objid)
        return objid


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
    def testBody(self, procs, output):
        response = self.createObject('oh', 'proximityTests/connectionTest.em');

class OneSSConnectionTest(ConnectionTest, OneSS):
    pass

class OneSSManualConnectionTest(ConnectionTest, OneManualSS):
    pass



class BasicQueryTest(ProximityTest):
    def testBody(self, procs, output):
        response = self.createObject('oh', 'proximityTests/basicQueryTest.em');

class OneSSBasicQueryTest(BasicQueryTest, OneSS):
    after = [OneSSConnectionTest]

class OneSSManualBasicQueryTest(BasicQueryTest, OneManualSS):
    after = [OneSSConnectionTest]
