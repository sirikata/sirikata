import sys
import server_graph

#do main loop through stats file
if (len(sys.argv) < 2):
    print "Must specify input file"
    sys.exit(-1)

dat_filename = sys.argv[1]

server_graph = server_graph.ServerGraph()

fp = open(dat_filename, 'r')
for line in fp:
    [server1, server2, time, bandwidth] = line.split(' ')
    server_graph.add(server1, server2, time, bandwidth)
fp.close()

server_graph.generate(dat_filename, False)
