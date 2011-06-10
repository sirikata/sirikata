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


def startPinto(**kwargs):
    args = [
        '--port=' + str(pinto_port),
        '--handler=rtreecut'
        ]
    server.RunPinto(args, **kwargs)



def startSpace(**kwargs):
    generate_ip_file()
    for ss in range(nservers):

        half_extents = [ block * num / 2 for block,num in zip(blocksize, layout)]
        neg = [ -x for x in half_extents ]
        pos = [ x for x in half_extents ]
        region = "<<%f,%f,%f>,<%f,%f,%f>>" % (neg[0]+center[0], neg[1]+center[1], neg[2]+center[2], pos[0]+center[0], pos[1]+center[1], pos[2]+center[2])
        layout_str = "<" + str(layout[0]) + "," + str(layout[1]) + "," + str(layout[2]) + ">"

        args = [
            '--id=%d' % (ss+1), # Server ID's start at 0, not one
            '--servermap=tabular',
            '--servermap-options=--filename=servermap.txt',
            '--layout=%s' % (layout_str),
            '--region=%s' % (region),
            '--pinto=master',
            '--pinto-options=' + '--host=' + str(pinto_ip) + ' --port=' + str(pinto_port),
            '--oseg=redis',
            '--oseg-options=' + '--prefix=' + str(oseg_prefix)
            ]
        server.RunSpace(ss, args, **kwargs)

def start(**kwargs):
    startPinto(**kwargs)
    time.sleep(5)
    startSpace(**kwargs)

start(with_xterm=True, debug=True, valgrind=False)
