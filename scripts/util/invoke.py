#/usr/bin/python
#
# invoke.py - Provides a method to invoke a subprocess using redirected input and output.

import subprocess
import time

def invoke(cmd, io=None):
    """Invoke the specified command and redirect IO using the specified IO object.

    Arguments:
    cmd -- array containing the command to execute and its arguments

    Keyword arguments:
    io -- IO object containing 3 file-like objects, or None to use normal IO objects

    """

    stdin_obj = None
    stdout_obj = None
    stderr_obj = None
    stdin_arg = None
    stdout_arg = None
    stderr_arg = None

    if io != None:
        stdin_obj = io.stdin
        stdout_obj = io.stdout
        stderr_obj = io.stderr

    if stdin_obj != None:
        stdin_arg = subprocess.PIPE
    if stdout_obj != None:
        stdout_arg = subprocess.PIPE
    if stderr_obj != None:
        stderr_arg = subprocess.PIPE


    sp = subprocess.Popen(cmd, stdin=stdin_arg, stdout=stdout_arg, stderr=stderr_arg)
    while( sp.returncode == None ):
        sp.poll()
        # FIXME note that we currently don't pipe in stdin because its not clear how
        # to do so without potentially blocking
        (stdoutdata, stderrdata) = sp.communicate()

        if stdoutdata != None:
            stdout_obj.write(stdoutdata)
        if stderrdata != None:
            stderr_obj.write(stderrdata)

        time.sleep(0.1)
