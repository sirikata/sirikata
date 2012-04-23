#!/usr/bin/env python

import subprocess

# This script manages running a single server instance given some
# configuration information.  It will usually be driven by a parent
# script which deals with common configuration and generating
# per-instance configuration options.


def AddStandardParams(appname, idx, args, env, **kwargs):
    if ('with_xterm' in kwargs and kwargs['with_xterm']):
        args.extend(['gnome-terminal', '-x'])
    if ('debug' in kwargs and kwargs['debug']):
        args.extend(['gdb', '-ex', 'run', '--args'])
    if ('valgrind' in kwargs and kwargs['valgrind']):
        args.extend(['valgrind'])

    if ('heapcheck' in kwargs and kwargs['heapcheck']):
        # Value should be the type of checking: minimal, normal,
        # strict, draconian
        env['HEAPCHECK'] = kwargs['heapcheck']
    elif ('heapprofile' in kwargs and kwargs['heapprofile']):
        # Value should be something like '/tmp' to indicate where we
        # should store heap profiling samples. In that example, the
        # samples will be stored into /tmp/appname.1.0001.heap,
        # /tmp/appname.1.0002.heap, etc.
        env['HEAPPROFILE'] = kwargs['heapprofile'] + '/' + appname + '.' + str(idx)

        if ('heapprofile_interval' in kwargs and kwargs['heapprofile_interval']):
            env['HEAP_PROFILE_ALLOCATION_INTERVAL'] = str(kwargs['heapprofile_interval'])


def RunApp(appname, idx, args, **kwargs):
    full_args = []
    full_env = {}
    AddStandardParams(appname, idx, full_args, full_env, **kwargs)
    full_args.extend(['./' + appname])
    full_args.extend(args)
    if 'plugins' in kwargs and kwargs['plugins'] is not None:
        full_args.extend([ kwargs['plugins'] ])
    print 'Running:', full_args, 'with environment:', full_env
    # Clear full_env if its empty
    if not full_env: full_env = None
    subprocess.Popen(full_args, env=full_env)

def RunPinto(args, **kwargs):
    RunApp('pinto_d', 0, args, **kwargs)

def RunSpace(ssid, args, **kwargs):
    RunApp('space_d', ssid, args, **kwargs)
