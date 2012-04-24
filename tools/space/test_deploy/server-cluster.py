#!/usr/bin/env python

# This script manages running a bunch of space servers to generate a single space.

import server
import socket
import time

nservers = 2
layout = (2, 1, 1)
port_base = 6666

hostname = socket.gethostname()

blocksize = (200, 8000, 8000)
center = (0,0,0)

pinto_ip = hostname
pinto_port = 6665

n_cseg_servers = 1
n_cseg_upper_servers = 1
cseg_ip = hostname
cseg_port_base = 6234

oseg_prefix = 'myspace-'


# Currently we need to generate all of these settings before starting up since
# they go into a file that is only read once
def generate_ip_file():
    f = open('servermap.txt', 'w')
    for ss in range(nservers):
        internal_port = port_base + 2*ss
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
    region, layout = get_region_and_layout()

    for cs in range(n_cseg_servers):
        args = [
            '--cseg-id=%d' % (cs+1),

            '--servermap=tabular',
            '--cseg-servermap-options=--filename=servermap.txt',

            '--layout=%s' % (layout),
            '--region=%s' % (region),

            '--cseg-service-tcp-port=' + str(cseg_port_base + cs),
            '--num-cseg-servers=' + str(n_cseg_servers),
            '--num-upper-tree-cseg-servers=' + str(n_cseg_upper_servers)
            ]
        server.RunCSeg(args, **kwargs)


def startPinto(**kwargs):
    args = [
        '--port=' + str(pinto_port),
        '--handler=rtreecut'
        ]
    server.RunPinto(args, **kwargs)


def startSpace(**kwargs):
    generate_ip_file()
    for ss in range(nservers):
        region, layout = get_region_and_layout()

        args = [
            '--id=%d' % (ss+1), # Server ID's start at 0, not one

            '--servermap=tabular',
            '--servermap-options=--filename=servermap.txt',

            '--layout=%s' % (layout),
            '--region=%s' % (region),

            '--cseg=client',
            '--cseg-service-host=' + str(cseg_ip),
            '--cseg-service-tcp-port=' + str(cseg_port_base + (ss % n_cseg_servers)),

            '--pinto=master',
            '--pinto-options=' + '--host=' + str(pinto_ip) + ' --port=' + str(pinto_port),

            '--oseg=redis',
            '--oseg-options=' + '--prefix=' + str(oseg_prefix)
            ]
        server.RunSpace(ss, args, **kwargs)

def start(**kwargs):
    startCSeg(**kwargs)
    startPinto(**kwargs)
    time.sleep(5)
    startSpace(**kwargs)

# Options:
# with_xterm - run services in terminals
# debug - run services in gdb
# valgrind - run services in valgrind
# heapcheck - turn on heap checking to the specified level
# heapprofile - turn on heap profiling, storing data to the specified destination
# heapprofile_interval - when heap profiling is on, take a snapshot every time this many bytes (accumulated over all allocations) are allocated (100MB by default)
start(with_xterm=True, debug=True, valgrind=False, heapcheck=False, heapprofile=False, heapprofile_interval=100*1024*1024)
