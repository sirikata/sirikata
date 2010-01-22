#!/usr/bin/python

import sys
import os
import os.path
import subprocess

CBR_WRAPPER = "scripts/util/cbr_wrapper.sh"

def FindRoot(start):
    for offset in ['.', '..', '../..', '../../..']:
        offset_dir = start + '/' + offset
        test_file = offset_dir + '/install-deps.sh'
        if (os.path.isfile(test_file)):
            return offset_dir
    return None

def RunCBR(args, **kwargs):
    # Get the *scripts* directory, then just run the wrapper
    root_dir = FindRoot( os.getcwd() )
    if root_dir == None:
        print "RunCBR: Couldn't find root directory from current directory."
        return

    cmd = [root_dir + '/' + CBR_WRAPPER]
    cmd.extend(args)
    subprocess.call(cmd, **kwargs)

if __name__ == "__main__":
    RunCBR(sys.argv)
