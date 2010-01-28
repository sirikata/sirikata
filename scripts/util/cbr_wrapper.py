#!/usr/bin/python

import sys
import os
import os.path
import invoke
import stdio

CBR_WRAPPER = "scripts/util/cbr_wrapper.sh"

def FindRoot(start):
    for offset in ['.', '..', '../..', '../../..']:
        offset_dir = start + '/' + offset
        test_file = offset_dir + '/install-deps.sh'
        if (os.path.isfile(test_file)):
            return offset_dir
    return None

def RunCBR(args, io=None, **kwargs):
    # Get the *scripts* directory, then just run the wrapper
    root_dir = FindRoot( os.getcwd() )
    if root_dir == None:
        print "RunCBR: Couldn't find root directory from current directory."
        return

    cmd = [root_dir + '/' + CBR_WRAPPER]
    cmd.extend(args)

    # Setup our IO, using default IO but overriding with parameters
    if io == None:
        io = stdio.StdIO()
    if ('stdin' in kwargs):
        io.stdin = kwargs['stdin']
        del kwargs['stdin']
    if ('stdout' in kwargs):
        io.stdout = kwargs['stdout']
        del kwargs['stdout']
    if ('stderr' in kwargs):
        io.stderr = kwargs['stderr']
        del kwargs['stderr']

    invoke.invoke(cmd, io=io, **kwargs)

if __name__ == "__main__":
    RunCBR(sys.argv)
