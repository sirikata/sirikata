import sys
import os
import subprocess
import random

#generate random colors
random.seed(0)
ServerGraphColors = []
for i in range(1,25):
    r = random.random()
    g = random.random()
    b = random.random()
    ServerGraphColors.append(str(r) + " " + str(g) + " " + str(b))


class ServerGraph:
    def __init__(self):
        self.graph_data = {}
        self.graph_command = ''

    def add(self, s1, s2, x, y):
        if (s1 not in self.graph_data):
            self.graph_data[s1] = {}

        server_graph_data = self.graph_data[s1]

        if (s2 not in server_graph_data):
            server_graph_data[s2] = []

        server_server_graph_data = server_graph_data[s2]
        server_server_graph_data.append( (x, y) )

    def begin_graph(self):
        self.graph_command = "newgraph\n"
        self.graph_command = self.graph_command + "xaxis size 5\n"
        self.graph_command = self.graph_command + "yaxis size 1\n"

    def begin_curve(self, index, name):
        self.graph_command = self.graph_command + "newcurve "
        self.graph_command = self.graph_command + "label : " + name + "\n"
        self.graph_command = self.graph_command + "marktype none linetype solid "
        self.graph_command = self.graph_command + "color " + ServerGraphColors[int(index)] + " "
        self.graph_command = self.graph_command + "pts\n"

    def generate_graph(self, out_filename):
        # and run it through jgraph
        out_fp = open(out_filename, 'w')
        sp = subprocess.Popen(['jgraph'], 0, None, subprocess.PIPE, out_fp, None, None, False, True)
        (soutdata, serrdata) = sp.communicate(self.graph_command)
        out_fp.close()

    def get_output_filename(self, prefix, sid):
        return prefix + "." + str(sid) + ".ps"

    def generate(self, prefix, with_self, with_sum):
        for (s1,d1) in self.graph_data.items():
            self.graph_command = ''
            self.begin_graph()
            sum = []

            for (s2,d2) in d1.items():
                if (not with_self and s1 == s2):
                    continue
                self.begin_curve(s2, "Server " + str(s2))
                idx = 0
                for (x,y) in d2:
                    self.graph_command = self.graph_command + str(x) + " " + str(y) + "\n"
                    if len(sum) <= idx:
                        sum.append((x,float(y)))
                    else:
                        # FIXME we should verify that the X values match
                        sum[idx] = (x, sum[idx][1] + float(y))
                    idx = idx + 1

            # generate the one last sum curve
            if with_sum:
                self.begin_curve(len(d1.items())+1, "Total")
                for (x,y) in sum:
                    self.graph_command = self.graph_command + str(x) + " " + str(y) + "\n"

            out_fname = self.get_output_filename(prefix, s1)

            self.generate_graph(out_fname)
