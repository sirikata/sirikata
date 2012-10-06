from __future__ import print_function
from test import Test
import random, json, time
import httplib, socket

class HttpCommandTest(Test):
    '''Utility class that provides settings and helper methods for
    interacting with a service driven by HTTP commands.
    '''

    def __init__(self, *args, **kwargs):
        super(HttpCommandTest, self).__init__(*args, **kwargs)
        self._nodes = {}

    def add_http(self, name):
        '''Add an http endpoint, i.e. a node you'll contact to execute commands'''
        self._nodes[name] = random.randint(3000, 4000)

    def http_settings(self, name):
        return [ '--command.commander=http',
                 '--command.commander-options=--port=' + str(self._nodes[name]),
                 ]

    def command(self, name, command, params=None, retries=None, wait=1):
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
                #print("HTTP command", command, "to", name, "failed:", str(exc), exc, file=self.output)
                return None


        # If we didn't finish successfull, indicate failure
        #print("HTTP command", command, "to", name, "failed after", tries, "tries", file=self.output)
        return None
