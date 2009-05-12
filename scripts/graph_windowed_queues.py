import sys
import server_graph

#do main loop through stats file
if (len(sys.argv) < 2):
    print "Must specify input file"
    sys.exit(-1)

dat_filename = sys.argv[1]

server_avg_queued_graph = server_graph.ServerGraph()
server_avg_weight_graph = server_graph.ServerGraph()

fp = open(dat_filename, 'r')
for line in fp:
    [server1, server2, time, avg_queued, avg_weight] = line.split(' ')
    server_avg_queued_graph.add(server1, server2, time, avg_queued)
    server_avg_weight_graph.add(server1, server2, time, avg_weight)
fp.close()

server_avg_queued_graph.generate("queued_" + dat_filename, False, True)
server_avg_weight_graph.generate("weight_" + dat_filename, False, True)
