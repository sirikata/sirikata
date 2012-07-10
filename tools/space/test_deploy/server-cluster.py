#!/usr/bin/env python

# This script manages running a bunch of space servers to generate a
# single space. It isn't intended to be very flexible, just a
# demonstration of how to run a multi-server space. It assumes it will
# use specific implementations for a variety of features -- Redis
# backed OSeg, a real Pinto server, HTTP commander enabled for easy
# debugging, etc. You can customize a bit by specifying an additional
# configuration file that will be loaded, but if you need to change
# any of the settings we have hard-coded, you should just use this
# script as a template.

import server
import socket
import time
from optparse import OptionParser

nservers = 2
layout = (2, 1, 1)
port_base = 6666

hostname = socket.gethostname()

blocksize = (4000, 8000, 8000)
center = (0,0,0)

pinto_ip = hostname
pinto_port = 6665

n_cseg_servers = 0
n_cseg_upper_servers = 1
cseg_ip = hostname
cseg_port_base = 6235
cseg_service_tcp_port = 6234 # The port space servers connect on
oseg_prefix = 'myspace-'

http_command_port_base = 9000

# Currently we need to generate all of these settings before starting up since
# they go into a file that is only read once
def generate_ip_file():
    f = open('servermap.txt', 'w')
    # And the real data
    for ss in range(nservers):
        internal_port = port_base + 2*ss
        external_port = internal_port + 1
        print >>f, hostname + ':' + str(internal_port) + ':' + str(external_port)
    f.close()

def generate_cseg_ip_file():
    # This servermap is used for cseg servers to contact each other (separate
    # from the port used for space servers to connect to them)
    f = open('cseg_servermap.txt', 'w')
    # And the real data
    for cs in range(n_cseg_servers):
        internal_port = cseg_port_base + 2*cs
        external_port = internal_port + 1
        print >>f, hostname + ':' + str(internal_port) + ':' + str(external_port)
    f.close()

def get_region_and_layout():
    half_extents = [ block * num / 2 for block,num in zip(blocksize, layout)]
    neg = [ -x for x in half_extents ]
    pos = [ x for x in half_extents ]
    region = "<<%f,%f,%f>,<%f,%f,%f>>" % (neg[0]+center[0], neg[1]+center[1], neg[2]+center[2], pos[0]+center[0], pos[1]+center[1], pos[2]+center[2])
    layout_str = "<" + str(layout[0]) + "," + str(layout[1]) + "," + str(layout[2]) + ">"

    return (region, layout_str)



def startCSeg(**kwargs):
    if n_cseg_servers == 0: return []

    generate_cseg_ip_file()
    region, layout = get_region_and_layout()

    ps = []
    for cs in range(n_cseg_servers):
        args = [
            # CSeg server IDs can't overlap with space server
            # IDs. Also, they must be the smallest IDs
            '--cseg-id=%d' % (cs+1),

            '--servermap=tabular',
            '--cseg-servermap-options=--filename=cseg_servermap.txt',

            '--layout=%s' % (layout),
            '--region=%s' % (region),

            '--cseg-service-tcp-port=' + str(cseg_service_tcp_port),
            '--num-cseg-servers=' + str(n_cseg_servers),
            '--num-upper-tree-cseg-servers=' + str(n_cseg_upper_servers)
            ]
        if 'cseg_config' in kwargs and kwargs['cseg_config']:
            args += [ '--cfg=' + kwargs['cseg_config'] ]
        ps += [ server.RunCSeg(args, **kwargs) ]
    return ps


def startPinto(**kwargs):
    args = [
        '--port=' + str(pinto_port),
        '--handler=rtreecut',

        '--command.commander=http',
        '--command.commander-options=--port=' + str(http_command_port_base + 100)
        ]
    if 'pinto_config' in kwargs and kwargs['pinto_config']:
        args += [ '--cfg=' + kwargs['pinto_config'] ]
    return [ server.RunPinto(args, **kwargs) ]


def startSpace(**kwargs):
    generate_ip_file()
    ps = []
    for ss in range(nservers):
        region, layout = get_region_and_layout()

        # Kind of gross, but to make it possible to select which type
        # of pinto config to use (e.g. if you use --pinto-config to
        # load pinto with the manual plugin), we need to filter out
        # the --pinto setting if the space config file specifies it
        no_pinto = False
        if 'space_config' in kwargs and kwargs['space_config']:
            with open(kwargs['space_config']) as f:
                no_pinto = any([x.startswith('pinto=') for x in f.readlines()])

        args = [
            '--id=%d' % (ss+1),

            '--servermap=tabular',
            '--servermap-options=--filename=servermap.txt',

            '--layout=%s' % (layout),
            '--region=%s' % (region),

            '--oseg=redis',
            '--oseg-options=' + '--prefix=' + str(oseg_prefix),

            '--command.commander=http',
            '--command.commander-options=--port=' + str(http_command_port_base + ss)
            ]
        if not no_pinto:
            args += ['--pinto=master']
        args += ['--pinto-options=' + '--host=' + str(pinto_ip) + ' --port=' + str(pinto_port)]

        if n_cseg_servers > 0:
            args += [
                '--cseg=client',
                '--cseg-service-host=' + str(cseg_ip),
                '--cseg-service-tcp-port=' + str(cseg_service_tcp_port),
                ]
        if 'space_config' in kwargs and kwargs['space_config']:
            args += [ '--cfg=' + kwargs['space_config'] ]
        ps += [ server.RunSpace(ss, args, **kwargs) ]
    return ps

def printOHTemplate(**kwargs):
    print
    print
    print "To start an object host connecting to this space, use at least the following parameters:"
    print
    print "    cppoh %s %s" % (
        '--servermap=tabular',
        '--servermap-options=--filename=servermap.txt',
        )
    print

def start(**kwargs):
    processes = []

    processes += startCSeg(**kwargs)
    processes += startPinto(**kwargs)
    time.sleep(5)
    processes += startSpace(**kwargs)

    printOHTemplate(**kwargs)

    if 'duration' in kwargs and kwargs['duration']:
        time.sleep(kwargs['duration'])
        for ps in processes:
            ps.terminate()
        for x in range(100):
            any_running = any([ps.returncode == None for ps in processes])
            if not any_running: break
            time.sleep(0.1)
        if any_running:
            for ps in processes:
                ps.kill()
                time.sleep(0.01)
                ps.kill()

parser = OptionParser()

parser.add_option("--sirikata", help="Path to sirikata binaries", action="store", type="str", dest="sirikata_path", default=None)

parser.add_option("--term", help="Run services in terminals", action="store_true", dest="with_xterm", default=True)
parser.add_option("--no-term", help="Don't run services in terminals", action="store_false", dest="with_xterm")
parser.add_option("--debug", help="Don't run in a debugger", action="store_true", dest="debug", default=True)
parser.add_option("--no-debug", help="Don't run in a debugger", action="store_false", dest="debug")
parser.add_option("--valgrind", help="Run under valgrind", action="store_true", dest="valgrind", default=False)
parser.add_option("--heap-check", help="Run with perftools heap check", action="store_true", dest="heapcheck", default=False)
parser.add_option("--heap-profile", help="Run with perftools heap profiling", action="store_true", dest="heapprofile", default=False)
parser.add_option("--heap-profile-interval", help="Frequency of heap snapshots (in bytes accumulated over all allocations)", action="store", type="int", dest="heapprofile_interval", default=100*1024*1024)

parser.add_option("--space-config", help="Extra configuration file to load on space servers", action="store", type="string", dest="space_config", default=None)
parser.add_option("--cseg-config", help="Extra configuration file to load on cseg servers", action="store", type="string", dest="cseg_config", default=None)
parser.add_option("--pinto-config", help="Extra configuration file to load on pinto servers", action="store", type="string", dest="pinto_config", default=None)

parser.add_option("--duration", help="Time to wait (and block) before killing child processes", action="store", type="int", dest="duration", default=None)

(options, args) = parser.parse_args()

# Options:
# with_xterm - run services in terminals
# debug - run services in gdb
# valgrind - run services in valgrind
# heapcheck - turn on heap checking to the specified level
# heapprofile - turn on heap profiling, storing data to the specified destination
# heapprofile_interval - when heap profiling is on, take a snapshot every time this many bytes (accumulated over all allocations) are allocated (100MB by default)
start(
    sirikata_path=options.sirikata_path,
    with_xterm=options.with_xterm,
    debug=options.debug,
    valgrind=options.valgrind,
    heapcheck=options.heapcheck,
    heapprofile=options.heapprofile,
    heapprofile_interval=options.heapprofile_interval,

    space_config=options.space_config,
    cseg_config=options.cseg_config,
    pinto_config=options.pinto_config,

    duration=options.duration
    )
