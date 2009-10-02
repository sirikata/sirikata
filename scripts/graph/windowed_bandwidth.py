#!/usr/bin/python

import sys
import server

# Graph windowed bandwidth for all servers from the given data file
def GraphWindowedBandwidth(filename):
    server_graph = server.ServerGraph()

    fp = open(filename, 'r')
    for line in fp:
        [server1, server2, time, bandwidth] = line.split(' ')
        server_graph.add(server1, server2, time, bandwidth)
    fp.close()

    server_graph.y_min = 0
    server_graph.generate(filename, False, True)


def graph_windowed_bandwidth_main():
    if (len(sys.argv) < 2):
        print "Must specify input file"
        sys.exit(-1)

    dat_filename = sys.argv[1]
    GraphWindowedBandwidth(dat_filename)

#do main loop through stats file
if __name__ == "__main__":
    graph_windowed_bandwidth_main()
