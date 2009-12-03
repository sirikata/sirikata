#!/usr/bin/python

import sys
import re
import stacked_bar

def graph_message_latency(log_files, filename=None):
    data_srcs = log_files

    group_pattern = '^Group: (.*)'
    stage_pattern = '^Stage (.*):(.*)s stddev (.*) #(.*)'

    # keep track of labels (trials)
    trial_set = set()
    # track stage => group, currently only used to get list of stages
    # could be useful for filtering by groups
    stage_group_map = {}
    # also keep track of a dict of (trial,stage) => (mean, stddev)
    data = {}

    for data_src in data_srcs:
        fp = open(data_src, 'r')

        trial = data_src
        trial_set.add(trial)

        current_group = 'Unknown'

        for line in fp:
            # Update current group
            group_match = re.search(group_pattern, line)
            if group_match:
                current_group = group_match.group(1)
                continue

            # Otherwise get stage data
            stage_match = re.search(stage_pattern, line)
            if stage_match:
                stage_name = stage_match.group(1)
                stage_avg = float(stage_match.group(2))
                stage_stddev = float(stage_match.group(3))
                stage_count = int(stage_match.group(4))

                #if stage_count < 10000:
                #    continue

                if (not stage_name in stage_group_map):
                    stage_group_map[stage_name] = current_group

                data[ (trial,stage_name) ] = (stage_avg*1000, stage_stddev*1000)

        fp.close()


    # Now that we've loaded all the data, transform it into a format that can be
    # passed to the stacked bar chart grapher
    ind = range(1,len(data_srcs)+1)
    labels = data_srcs


    stage_labels = []
    avgs = []
    stddevs = []
    for stage in stage_group_map.keys():
        stage_labels.append( stage )

        avg_vec = []
        stddev_vec = []

        for trial in data_srcs:
            data_key = (trial, stage)
            if data_key not in data:
                avg = 0
                stddev = 0
            else:
                avg,stddev = data[ data_key ]

            avg_vec.append(avg)
            stddev_vec.append(stddev)

        avgs.append(avg_vec)
        stddevs.append(stddev_vec)

    chart = stacked_bar.stacked_bar('Stage Latencies', 'Group', 'Latency (ms)', ind, labels, avgs, stddevs, stage_labels)

#    chart.show()
    if filename == None:
        filename = 'latency_stacked_bar.pdf'
    chart.savefig(filename)


#do main loop through stats file
if __name__ == "__main__":
    graph_message_latency(sys.argv[1:])
