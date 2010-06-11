#!/usr/bin/python

import sys
import os
import os.path
import invoke
import stdio

CBR_WRAPPER = "scripts/util/cbr_wrapper.sh"

def FindRoot(start):
    search_offsets = ['.', '..', '../..', '../../..']
    # Search directories: locals preferred first
    search_dirs = [os.getcwd()]
    # Otherwise search system dirs
    search_dirs.extend(sys.path)
    for sdir in search_dirs:
        for soffset in search_offsets:
            test_file = os.path.join(sdir, soffset, 'install-deps.sh')
            if (os.path.isfile(test_file)):
                return os.path.join(sdir, soffset)
    return None

def RunCBR(args, io=None, **kwargs):
    # Get the *scripts* directory, then just run the wrapper
    root_dir = FindRoot( os.getcwd() )
    if root_dir == None:
        print "RunCBR: Couldn't find root directory from current directory."
        return

    cmd = [os.path.join(root_dir, CBR_WRAPPER)]
    cmd.extend(args)

    # Setup our IO, using default IO but overriding with parameters
    if io == None:
        io = stdio.StdIO()
    our_io = stdio.StdIO(io.stdin, io.stdout, io.stderr)
    if ('stdin' in kwargs):
        our_io.stdin = kwargs['stdin']
        del kwargs['stdin']
    if ('stdout' in kwargs):
        our_io.stdout = kwargs['stdout']
        del kwargs['stdout']
    if ('stderr' in kwargs):
        our_io.stderr = kwargs['stderr']
        del kwargs['stderr']

    invoke.invoke(cmd, io=our_io, **kwargs)

if __name__ == "__main__":
    RunCBR(sys.argv)
