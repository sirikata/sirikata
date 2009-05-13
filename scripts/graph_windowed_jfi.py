import sys
import server_graph

#do main loop through stats file
if (len(sys.argv) < 2):
    print "Must specify input file"
    sys.exit(-1)

bandwidth_filename = sys.argv[1]
queue_filename = sys.argv[2]
output_filename = sys.argv[3]

bandwidth_fp = open(bandwidth_filename, 'r')
queue_fp = open(queue_filename, 'r')

# read in all the data, generating the appropriate tuples
data = {}
for bandwidth_line in bandwidth_fp:
    queue_line = queue_fp.readline()
    [bandwidth_sender, bandwidth_receiver, bandwidth_time, bandwidth] = bandwidth_line.split(' ')
    [queue_sender, queue_receiver, queue_time, avg_queued, avg_weight] = queue_line.split(' ')

    if (bandwidth_sender != queue_sender) or (bandwidth_receiver != queue_receiver) or (bandwidth_time != queue_time):
        print "Mismatch in file data, aborting"
        exit(-1)

    sender = bandwidth_sender
    receiver = bandwidth_receiver
    time = int(bandwidth_time)
    bandwidth = float(bandwidth)
    avg_queued = float(avg_queued)
    avg_weight = float(avg_weight)

    weight_by_queued = avg_weight
    if (avg_queued == 0):
        weight_by_queued = 0

    if not sender in data:
        data[sender] = []

    data[sender].append( (time, bandwidth, weight_by_queued) )

bandwidth_fp.close()
queue_fp.close()

if (len(data) == 0):
    exit(0)

# loop through all data, while the time step is the same, continue the JFI computatation
# when it changes generate a new data point for the graph
server_jfi_graph = server_graph.ServerGraph()

for (sender, sender_data) in data.items():
    print sender, len(sender_data)
    sender_data.sort(key = lambda x:x[0]) # sort by time
    last_time = -1
    for (time, bandwidth, weight) in sender_data:
        if time != last_time:
            if last_time != -1: # generate a data point if appropriate
                jfi = 0
                if count != 0 and sum_x_i_squared != 0:
                    jfi = (sum_x_i*sum_x_i) / (float(count) * sum_x_i_squared)
                server_jfi_graph.add(sender, 0, time, jfi)

            # reset per-time info
            sum_x_i = 0
            sum_x_i_squared = 0
            count = 0

        if weight != 0:
            x_i = bandwidth / weight
            sum_x_i += x_i
            sum_x_i_squared += x_i*x_i
            count += 1

        last_time = time

print output_filename
server_jfi_graph.generate(output_filename, False, False)
