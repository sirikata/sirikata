#!/usr/bin/env python

from __future__ import print_function
import sys
import subprocess
import datetime
import time
import signal


class ProcSet:
    """
    ProcSet tracks a set of processes for running a test. It's mostly
    a wrapper around subprocess that also handles waiting for the
    processes to exit. It will forcibly kill the processes if they
    don't exit.
    """

    def __init__(self):
        self._procRequests = []
        self._defaultProc = 0

    def process(self, *args, **kwargs):
        # Just queue it up. We'll do the real work in run. Extract a
        # few special arguments we might need
        # at -- number of seconds to wait before starting this process
        atTime = 0
        if 'at' in kwargs:
            atTime = kwargs['at']
            del kwargs['at']

        if 'default' in kwargs and kwargs['default']:
            self._defaulProc = len(self._procRequests)
            del kwargs['default']
        self._procRequests.append( (args, kwargs, atTime) )

    def run(self, waitUntil=None, killAt=None, output=sys.stdout):
        """
        Run the processes.

        waitUntil -- wait this long for the processes to run and exit
           before sending an initial kill request.
        killAt -- seconds until sending a SIGKILL to the processes.
        """

        def allTerminated(ps):
            for p in ps:
                if p is None or p.poll() is None: return False
            return True

        start = datetime.datetime.now()
        self._procs = [None] * len(self._procRequests)

        self._hupped = [False] * len(self._procRequests)
        self._killed = [False] * len(self._procRequests)

        sighupped = False # Whether we've sent the sighup yet
        while not allTerminated(self._procs):
            now = (datetime.datetime.now() - start).seconds

            # Run anything that's now ready to run
            for idx in range(len(self._procRequests)):
                (args, kwargs, atTime) = self._procRequests[idx]
                if self._procs[idx] is None and atTime < now:
                    self._procs[idx] = subprocess.Popen(*args, **kwargs)

            # If we've hit the wait time, send requested signals to
            # processes that are still alive
            if waitUntil is not None and waitUntil < now and not sighupped:
                # SIGHUP processes that haven't shutdown yet
                print('Sending shutdown signals to processes', file=output);
                sys.stdout.flush();
                sys.stderr.flush();
                output.flush()
                for idx in range(len(self._procRequests)):
                    if self._procs[idx] is not None and self._procs[idx].poll() is None:
                        self._procs[idx].send_signal(signal.SIGHUP)
                        self._hupped[idx] = True
                sighupped = True

            # If we've hit the kill time, forcibly kill them
            if killAt is not None and killAt < now:
                print('Sending shutdown kill signals to processes', file=output);
                sys.stdout.flush();
                sys.stderr.flush();
                output.flush()
                for idx in range(len(self._procRequests)):
                    if self._procs[idx] is not None and self._procs[idx].poll() is None:
                        # Send signal twice, in case the first one is
                        # caught. The second should (in theory) always
                        # kill the process as it should have disabled
                        # the handler
                        self._procs[idx].send_signal(signal.SIGKILL)
                        self._procs[idx].send_signal(signal.SIGKILL)
                        self._procs[idx].wait()
                        self._killed[idx] = True

            # Sleep so it's not busy waiting for the run
            time.sleep(1)

    def hupped(self, idx=None):
        """
        Return true if the given process (by index) was SIGHUPped. If
        no index is specified, returns the default process's result.
        """
        if idx is None: idx = self._defaultProc
        return self._hupped[idx]

    def killed(self, idx=None):
        """
        Return true if the given process (by index) was SIGKILLed. If
        no index is specified, returns the default process's result.
        """
        if idx is None: idx = self._defaultProc
        return self._killed[idx]

    def returncode(self, idx=None):
        """
        Return the exit code for the given process (by index). If no
        index is specified, returns the default process's result.
        """
        if idx is None: idx = self._defaultProc
        return self._procs[idx].returncode
