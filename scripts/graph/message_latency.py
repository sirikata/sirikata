#!/usr/bin/python

import sys
import re
import stacked_bar

# insert_after is a helper for maintaining an ordered list.
def insert_after(ordered_list, new_val, prev_val):
    if prev_val == None:
        ordered_list.insert(0, new_val)
    else:
        idx = ordered_list.index(prev_val)
        if idx == -1:
            ordered_list.append(new_val)
        else:
            ordered_list.insert(idx+1, new_val)


# Inserts line breaks into a name to make it fit nicely into a legend
def line_break_stage_name(name, max_chars = 40):
    split = name.split(' ')
    words = [x for x in split if len(x) > 1]
    return " ->\n".join(words)

# Converts a string containing a time, of the form 123u (microseconds),
# 123m (milliseconds) or 123 (seconds) to an int # of microseconds
def time_to_microseconds(str_time):
    if len(str_time) == 0:
        return 0

    # If we don't have an s at the end our only chance is to assume its
    # raw seconds
    last = str_time[-1:]
    if last != 's':
        return int( float(str_time) * 1000000 )

    # Otherwise, strip the s and check second to last
    str_time = str_time[:-1]
    last = str_time[-1:]

    if last == 'u':
        return int( float(str_time[:-1]) )
    elif last == 'm':
        return int( float(str_time[:-1]) * 1000 )
    else:
        return int( float(str_time) * 1000000 )

def graph_message_latency(log_files, filename=None):
    data_srcs = log_files

    group_pattern = '^Group: (.*)'
    stage_pattern = '^Stage (.*):(.*s) stddev (.*s) #(.*)'

    # keep track of labels (trials)
    trial_set = set()
    # track stage => group, currently only used to get list of stages
    # could be useful for filtering by groups
    stage_group_map = {}
    ordered_stage_names = []
    # also keep track of a dict of (trial,stage) => (mean, stddev)
    data = {}

    for data_src in data_srcs:
        fp = open(data_src, 'r')

        trial = data_src
        trial_set.add(trial)

        current_group = 'Unknown'
        prev_stage = None

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
                stage_avg = time_to_microseconds(stage_match.group(2))
                stage_stddev = time_to_microseconds(stage_match.group(3))
                stage_count = int(stage_match.group(4))

                if stage_count < 10:
                    continue

                if (not stage_name in stage_group_map):
                    stage_group_map[stage_name] = current_group
                    insert_after(ordered_stage_names, stage_name, prev_stage)

                data[ (trial,stage_name) ] = (stage_count, stage_avg, stage_stddev)

                prev_stage = stage_name

        fp.close()


    # Now that we've loaded all the data, transform it into a format that can be
    # passed to the stacked bar chart grapher
    ind = range(1,len(data_srcs)+1)
    labels = data_srcs

    stage_labels = []
    avgs = []
    stddevs = []
    widths = []

    for stage in ordered_stage_names:
        stage_labels.append( line_break_stage_name(stage) )

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
                #width=math.log(width);
            width_vec.append(width);
            avg_vec.append(avg)
            #stddev_vec.append(stddev)
            stddev_vec.append(0)

        avgs.append(avg_vec)
        stddevs.append(stddev_vec)
        widths.append(width_vec)
    chart = stacked_bar.stacked_bar('Stage Latencies', 'Group', 'Latency (us)', ind, labels, widths, avgs, stddevs, stage_labels)

#    chart.show()
    if filename == None:
        filename = 'latency_stacked_bar.pdf'
    chart.savefig(filename)


#do main loop through stats file
if __name__ == "__main__":
    graph_message_latency(sys.argv[1:])
