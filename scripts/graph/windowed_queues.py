#!/usr/bin/python

import sys
import server

# Graph windowed queues for all servers from the given data file
def GraphWindowedQueues(filename):
    server_avg_queued_graph = server.ServerGraph()
    server_avg_weight_graph = server.ServerGraph()

    fp = open(filename, 'r')
    for line in fp:
        [server1, server2, time, avg_queued, avg_weight] = line.split(' ')
        server_avg_queued_graph.add(server1, server2, time, avg_queued)
        server_avg_weight_graph.add(server1, server2, time, avg_weight)
    fp.close()

    server_avg_queued_graph.y_min = 0

    server_avg_weight_graph.y_range(0, 1)

    server_avg_queued_graph.generate("queued_" + filename, False, False)
    server_avg_weight_graph.generate("weight_" + filename, False, False)


def graph_windowed_bandwidth_main():
    if (len(sys.argv) < 2):
        print "Must specify input file"
        sys.exit(-1)

    dat_filename = sys.argv[1]
    GraphWindowedQueues(dat_filename)

#do main loop through stats file
if __name__ == "__main__":
    graph_windowed_queues_main()
