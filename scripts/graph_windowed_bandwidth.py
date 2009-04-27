import sys
import os
import subprocess
import random

curserver1 = 0
curserver2 = 0
graph_command = ""
sum = []

def begin_graph(graph_command):
    graph_command = "newgraph\n"
    graph_command = graph_command + "xaxis size 5\n"
    graph_command = graph_command + "yaxis size 1\n"
    return graph_command

def begin_curve(graph_command, index, name):
    graph_command = graph_command + "newcurve "
    graph_command = graph_command + "label : " + name + "\n"
    graph_command = graph_command + "marktype none linetype solid "
    graph_command = graph_command + "color " + graph_colors[int(index)] + " "
    graph_command = graph_command + "pts\n"
    return graph_command


def update_sum(sum, time, bandwidth):
    for i in range(0,len(sum)-1):
        (t,b) = sum[i]
        if (time == t):
            sum[i] = (t, b + float(bandwidth))
            return None
    sum.append( (time, float(bandwidth)) )
    return None

def generate_graph(out_filename, graph_command):
    # and run it through jgraph
    out_fp = open(out_filename, 'w')
    sp = subprocess.Popen(['jgraph'], 0, None, subprocess.PIPE, out_fp, None, None, False, True)
    (soutdata, serrdata) = sp.communicate(graph_command)
    out_fp.close()
    #print graph_command

def finish_graph(out_filename, graph_command):
    # generate the one last sum curve
    graph_command = begin_curve(graph_command, int(curserver2)+1, "Total")
    for (time,bandwidth) in sum:
        graph_command = graph_command + time + " " + str(bandwidth) + "\n"
    generate_graph(out_filename, graph_command)

def get_output_filename(source_fn, sid):
    return source_fn + "." + str(sid) + ".ps"

#generate random colors
random.seed(0)
graph_colors = []
for i in range(1,25):
    r = random.random()
    g = random.random()
    b = random.random()
    graph_colors.append(str(r) + " " + str(g) + " " + str(b))

#do main loop through stats file
if (len(sys.argv) < 2):
    print "Must specify input file"
    sys.exit(-1)

dat_filename = sys.argv[1]
fp = open(dat_filename, 'r')

for line in fp:
    [server1, server2, time, bandwidth] = line.split(' ')

    if (server1 != curserver1):
        if (curserver1 != 0):
            out_fname = get_output_filename(dat_filename, curserver1)
            finish_graph(out_fname, graph_command)
        sum = []
        graph_command = begin_graph(graph_command)

    if (server2 != curserver2):
        if (server1 != server2):
            graph_command = begin_curve(graph_command, server2, "Server " + server2)

    curserver1 = server1
    curserver2 = server2

    if (curserver1 != curserver2):
        update_sum(sum, time, bandwidth)
        graph_command = graph_command + time + " " + str(bandwidth) + "\n"

# the last graph needs to be output still since the last one won't have a next line with a server change
out_fname = get_output_filename(dat_filename, curserver1)
finish_graph(out_fname, graph_command)

fp.close()
