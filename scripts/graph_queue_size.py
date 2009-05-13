import sys
import server_graph

#do main loop through stats file
if (len(sys.argv) < 2):
    print "Must specify input file"
    sys.exit(-1)

dat_filename = sys.argv[1]

send_server_graph = server_graph.ServerGraph()
receive_server_graph = server_graph.ServerGraph()

fp = open(dat_filename, 'r')
for line in fp:
    [server1, server2, time, send_size, send_queued, send_weight, receive_size, receive_queued, receive_weight] = line.split(' ')
    send_server_graph.add(server1, server2, time, send_queued)
    receive_server_graph.add(server1, server2, time, receive_queued)
fp.close()

send_server_graph.y_min = 0
receive_server_graph.y_min = 0

send_server_graph.generate("send_" + dat_filename, False, False)
receive_server_graph.generate("receive_" + dat_filename, False, False)
