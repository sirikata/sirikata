from __future__ import print_function
from http import HttpCommandTest
import os.path, subprocess, traceback
from framework.procset import ProcSet

class WorldHttpTest(HttpCommandTest):
    '''
    Runs an entire system (cseg, pinto, space servers and cppoh), but
    only the services actually requested. This class provides
    utilities for running the system, selecting the type of query
    system to use, and for adding and removing objects during tests.
    '''

    duration = 30
    wait_until_responsive = True
    script_paths = [ os.path.dirname(__file__) ]

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

    def runTest(self):
        procs = ProcSet()
        self.report_files = {}
        self._outputs = {}
        for svc_idx,svc in enumerate(self.services):
            svc_name = svc['name']

            service_output_filename = os.path.join(self._folder, svc_name + '.log')

            # Add with http port
            self.add_http(svc_name)
            service_cmd = [self._binaries[svc['type']]]
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
                result = self.command(svc_name, 'meta.commands', retries=40, wait=.25)
                if result is None: self.fail('Server never became responsive to HTTP commands')

        # Defer to the implementation for sending and checking test
        # commands. Catch all errors so we can make sure we cleanup the
        # processes properly and report crashes, etc.
        try:
            self.testBody()
        except:
            self.fail(msg='Uncaught exception during test:\n' + traceback.format_exc())

        # The implementation should know whether things are shutting
        # down cleanly. We just make sure we clean up aggressively if
        # necessary.
        if not procs.done():
            procs.wait(until=self.duration, killAt=self.duration, output=self.output)

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
        print("Execution Log:", file=self.output)
        for report_name, report_file in self.report_files.iteritems():
            print("  ", report_name, file=self.output)
            fp = open(report_file, 'r')
            for line in fp.readlines():
                print("    ", line, end='', file=self.output)
            fp.close()
            print(file=self.output)


    def createObject(self, oh, script_file):
        '''
        Helper for tests that creates an object on the given OH. The
        script should be a filename (under system/test/). Returns
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
