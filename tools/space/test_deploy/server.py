#!/usr/bin/env python

import subprocess

# This script manages running a single server instance given some
# configuration information.  It will usually be driven by a parent
# script which deals with common configuration and generating
# per-instance configuration options.

# Some static configuration options for convenience

plugins = {
    'core' : 'tcpsst,servermap-tabular,core-local,graphite',
    'space' : 'weight-exp,weight-sqr,weight-const,space-null,space-craq,space-local,space-standard,space-master-pinto,colladamodels,space-redis',
    'cseg' : 'weight-exp,weight-sqr,weight-const',
    'pinto' : '',
    'oh' : 'weight-exp,weight-sqr,weight-const,tcpsst,sqlite,weight-const,ogregraphics,colladamodels,csvfactory,scripting-js,simplecamera'
}


def AddStandardParams(args, **kwargs):
    if ('with_xterm' in kwargs and kwargs['with_xterm']):
        args.extend(['gnome-terminal', '-x'])
    if ('debug' in kwargs and kwargs['debug']):
        args.extend(['gdb', '-ex', 'run', '--args'])
    if ('valgrind' in kwargs and kwargs['valgrind']):
        args.extend(['valgrind'])

def RunApp(appname, plugins, args, **kwargs):
    full_args = []
    AddStandardParams(full_args, **kwargs)
    full_args.extend([appname])
    full_args.extend(args)
    if plugins:
        full_args.extend([ plugins ])
    print 'Running:', full_args
    subprocess.Popen(full_args)

def RunPinto(args, **kwargs):
    RunApp('./pinto_d', None, args, **kwargs)

def RunSpace(ssid, args, **kwargs):
    RunApp('./space_d', '--space.plugins=' + plugins['space'], args, **kwargs)
