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

    class Proc(object):
        '''Properties of a process being tracked by this ProcSet'''
        pass

    def __init__(self):
        self._procs = set()
        self._defaultProc = None

        # Time we started the first process
        self._start = None
        # Whether we've sent sighups out to the processes yet
        self._sighupped = False

    def process(self, *args, **kwargs):
        """
        Start a process running

        default -- use this as the default process when checking for
           sighups, sigkills and returncodes
        wait -- whether to wait for this process to exit before
           sending sighups. True by default.
        """
        # Just queue it up. We'll do the real work in run. Extract a
        # few special arguments we might need

        set_default = False
        if 'default' in kwargs and kwargs['default']:
            set_default = True
            del kwargs['default']

        wait = True
        if 'wait' in kwargs and not kwargs['wait']:
            wait = False
            del kwargs['wait']

        p = ProcSet.Proc()
        p.args = args
        p.kwargs = kwargs
        p.wait = wait
        if not self._start:
            self._start = datetime.datetime.now()
        p.proc = subprocess.Popen(*args, **kwargs)
        p.hupped = False
        p.killed = False
        self._procs.add(p)

        if set_default:
            self._defaultProc = p

    def sleep(self, duration):
        # TODO(ewencp) maybe poll, returning early if everything exits?
        time.sleep(duration)

    def _allTerminated(self):
        for p in self._procs:
            if p.proc.poll() is None: return False
        return True

    def waitingDone(self):
        '''Returns True if the processes we marked as needing to exit themselves have terminated'''
        for p in self._procs:
            if p.wait and p.proc.poll() is None:
                return False
        return True

    def done(self):
        '''Returns True if all processes have terminated'''
        return self._allTerminated()

    def hup(self, output=sys.stdout):
        # SIGHUP processes that haven't shutdown yet
        sys.stdout.flush();
        sys.stderr.flush();
        output.flush()
        for p in self._procs:
            if p.proc.poll() is None:
                p.proc.send_signal(signal.SIGHUP)
                p.hupped = True
        self._sighupped = True

    def kill(self, output=sys.stdout):
        sys.stdout.flush();
        sys.stderr.flush();
        output.flush()
        for p in self._procs:
            if p.proc.poll() is None:
                # Send signal twice, in case the first one is
                # caught. The second should (in theory) always
                # kill the process as it should have disabled
                # the handler
                p.proc.send_signal(signal.SIGKILL)
                p.proc.send_signal(signal.SIGKILL)
                p.proc.wait()
                p.killed = True

    def wait(self, until=None, killAt=None, output=sys.stdout):
        """
        Wait for running processes to exit.

        until -- wait this long for the processes to run and exit
           before sending an initial kill request. This time is
           relative to the *starting time*, the time when the first
           process was spawned, not the current time.
        killAt -- seconds until sending a SIGKILL to the processes.
        """

        while not self._allTerminated():
            now_dt = datetime.datetime.now() - self._start
            now = now_dt.seconds + (float(now_dt.microseconds)/1000000)

            # If we've hit the wait time, send requested signals to
            # processes that are still alive
            done_waiting = until is not None and until < now
            all_waiting_exited = self.waitingDone()
            if (done_waiting or all_waiting_exited) and not self._sighupped:
                self.hup(output=output)

            # If we've hit the kill time, forcibly kill them
            if killAt is not None and killAt < now:
                self.kill(output=output)

            # Sleep so it's not busy waiting for the run
            time.sleep(0.1)

    def hupped(self):
        """
        Return true if the given process (by index) was SIGHUPped. If
        no index is specified, returns the default process's result.
        """
        if not self._defaultProc:
            raise RuntimeError("No process marked as the default, can't determine if it was sent a SIGHUP")
        return self._defaultProc.hupped

    def killed(self, idx=None):
        """
        Return true if the given process (by index) was SIGKILLed. If
        no index is specified, returns the default process's result.
        """
        if not self._defaultProc:
            raise RuntimeError("No process marked as the default, can't determine if it was sent a SIGKILL")
        return self._defaultProc.killed

    def returncode(self):
        """
        Return the exit code for the given process (by index). If no
        index is specified, returns the default process's result.
        """
        if not self._defaultProc:
            raise RuntimeError("No process marked as the default, can't determine if it was sent a SIGKILL")
        return self._defaultProc.proc.returncode
