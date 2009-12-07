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

                if stage_count < 1000:
                    continue

                if (not stage_name in stage_group_map):
                    stage_group_map[stage_name] = current_group

                data[ (trial,stage_name) ] = (stage_count, stage_avg*1000, stage_stddev*1000)

        fp.close()


    # Now that we've loaded all the data, transform it into a format that can be
    # passed to the stacked bar chart grapher
    ind = range(1,len(data_srcs)+1)
    labels = data_srcs


    stage_labels = []
    avgs = []
    stddevs = []
    widths = []
    ordered_stages=[]
    unordered_stages={}
    did_process={}
    current_matches={"CREATED":True};
    next_matches={}
    any_matches=True;
    while any_matches:
        any_matches=False
        for stage in stage_group_map.keys():
            st,ed=stage.split("-");
            if (st in current_matches):
                any_matches=True;
                if not stage in unordered_stages:
                    ordered_stages.append(stage);
                    unordered_stages[stage]=True;
                    if (ed!=st and not ( ed in did_process)):
                        next_matches[ed]=True
        for match in current_matches:
            did_process[match]=True
        current_matches=next_matches;
        next_matches={}

    for stage in ordered_stages:
        stage_labels.append( stage )

        avg_vec = []
        stddev_vec = []
        width_vec = []

        for trial in data_srcs:
            data_key = (trial, stage)
            if data_key not in data:
                avg = 0
                stddev = 0
                width=0
            else:
                import math
                width,avg,stddev = data[ data_key ]
                width=math.log(width);
            width_vec.append(width);
            avg_vec.append(avg)
            stddev_vec.append(stddev)

        avgs.append(avg_vec)
        stddevs.append(stddev_vec)
        widths.append(width_vec)
    chart = stacked_bar.stacked_bar('Stage Latencies', 'Group', 'Latency (ms)', ind, labels, widths, avgs, stddevs, stage_labels)

#    chart.show()
    if filename == None:
        filename = 'latency_stacked_bar.pdf'
    chart.savefig(filename)


#do main loop through stats file
if __name__ == "__main__":
    graph_message_latency(sys.argv[1:])
