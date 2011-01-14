#!/usr/bin/env python

import sys
import subprocess

host = 'localhost'
port = 6880
debug = False

def parse(args):
    global host, port, debug
    for arg in args:
        if arg == '--debug':
            debug = True
        elif arg.startswith('--host='):
            host = arg.replace('--host=', '', 1);
        elif arg.startswith('--port='):
            port = int( arg.replace('--port=', '', 1) )

def run(cmd, debug):
    if not debug:
        return subprocess.Popen(cmd)

    debug_cmd = ['gdb', '--args']
    debug_cmd.extend(cmd)
    return subprocess.Popen(debug_cmd)
