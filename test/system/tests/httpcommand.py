#!/usr/bin/python

from __future__ import print_function
from framework.tests.test import Test
from framework.db.entity import Entity
import sys, os, random, json, subprocess, time
import httplib, socket
from framework.procset import ProcSet

class HttpCommandTest(Test):
    def __init__(self):
        super(HttpCommandTest, self).__init__()
        self._nodes = {}

    def add(self, name):
        '''Add an http endpoint, i.e. a node you'll contact to execute commands'''
        self._nodes[name] = random.randint(3000, 4000)

    def http_settings(self, name):
        return [ '--command.commander=http',
                 '--command.commander-options=--port=' + str(self._nodes[name]),
                 ]

    def command(self, name, command, params=None, retries=None, wait=1, output=sys.stdout):
        '''
        Execute the given command, with optional JSON params, against the node. Return the response.

        name -- name of process to send command to
        command -- command name, e.g. 'meta.commands'
        params -- options, in form of dict to be JSON encoded
        retries -- number of times to retries if there's a connection failure
        wait -- time to wait between retries
        '''

        tries = 0
        max_retries = (retries or 1)
        while tries < max_retries:
            try:
                result = None
                conn = httplib.HTTPConnection('localhost', self._nodes[name], timeout=5)
                body = (params and json.dumps(params)) or None
                conn.request("POST", "/" + command, body=body)
                resp = conn.getresponse()
                if resp.status == 200:
                    result = json.loads(resp.read())
                conn.close()
                return result
            except socket.error:
                tries += 1
                if wait:
                    time.sleep(wait)
            except httplib.HTTPException, exc:
                # Got connection, but it failed. Log and indicate failure
                #print("HTTP command", command, "to", name, "failed:", str(exc), exc, file=output)
                return None


        # If we didn't finish successfull, indicate failure
        #print("HTTP command", command, "to", name, "failed after", tries, "tries", file=output)
        return None



class SpaceHttpCommandTest(HttpCommandTest):
    '''Runs a space server and checks that '''

    duration = 30
    wait_until_responsive = True

    def runTest(self, outputPath, binPath, cppohBinName, spaceBinName, output=sys.stdout):
        space_output_filename = os.path.join(outputPath, 'space.log')

        # Space
        self.add('space')
        space_cmd = [os.path.join(binPath, spaceBinName)]
        space_cmd += self.http_settings('space')

        procs = ProcSet()
        space_output = open(space_output_filename, 'w')
        print(' '.join(space_cmd), file=space_output)
        space_output.flush()
        procs.process(space_cmd, stdout=space_output, stderr=subprocess.STDOUT, default=True)

        # If requested, block for upto 10 seconds for the process to start responding to commands
        if self.wait_until_responsive:
            result = self.command('space', 'meta.commands', retries=40, wait=.25, output=output)
            if result is None: self.fail('Space server never became responsive to HTTP commands')

        # Defer to the implementation for sending and checking test commands
        self.testCommands(procs, output=output)

        # The implementation should know whether things are shutting
        # down cleanly. We just make sure we clean up aggressively if
        # necessary.
        if not procs.done():
            procs.wait(until=self.duration, killAt=self.duration, output=output)

        # Print a notification if we had to kill this process
        if procs.killed():
            self.fail('Timed out')

        space_output.close()
        self.report_files = {'Space Server': space_output_filename}

        self.assertLogErrorFree(space_output_filename)
        self.assertReturnCode(procs.returncode())

    def report(self):
        print("Execution Log:", file=self._output)
        for report_name, report_file in self.report_files.iteritems():
            print("  ", report_name, file=self._output)
            fp = open(report_file, 'r')
            for line in fp.readlines():
                print("    ", line, end='', file=self._output)
            fp.close()
            print(file=self._output)

class SpaceMetaCommandsTest(SpaceHttpCommandTest):
    '''
    Sanity check that the command processor was instantiated and works
    for the simplest command that is always guaranteed to work
    '''
    duration = 10

    # The one test where we don't want to block until a command gets through
    wait_until_responsive = False

    def testCommands(self, procs, output=sys.stdout):
        # meta.commands is always available, even if empty
        result = self.command('space', 'meta.commands', retries=20, wait=.25, output=output)
        self.assertIsNotNone(result)
        procs.hup(output=output)

class SpaceShutdownTest(SpaceHttpCommandTest):
    '''
    Sanity check that the command processor was instantiated and works
    for the simplest command that is always guaranteed to work
    '''

    after = [SpaceMetaCommandsTest]

    def testCommands(self, procs, output=sys.stdout):
        # meta.commands is always available, even if empty
        self.assertIsNotNone( self.command('space', 'context.shutdown', output=output) )
