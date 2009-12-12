#!/usr/bin/python

import sys
import re
import stacked_bar

# FIXME It would be nice to have a better way of making this script able to find
# other modules in sibling packages
sys.path.insert(0, sys.path[0]+"/..")

import util.time_util

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


# breaks a single string into parts which will fit into the specified number of chars
def break_string(name, max_chars):
    # base case
    if (len(name) <= max_chars):
        return [name]

    # otherwise, find a good place to split and recurse
    for split_chars in [
        [' '],
        ['>'],
        ['_']
        ]:
        split_chars = set(['_', '-'])
        splits = [x for x in range(0,len(name)-1) if name[x] in split_chars]
        if len(splits) == 0:
            continue

        # Since we found one, we need to pick the best one
        # Choose either the longest possible prefix to cut off
        # or the longest possible postfix
        early_offsets = [x for x in splits if x < max_chars]
        max_early_offset = (len(early_offsets) > 0) and max(early_offsets) or 0
        late_offsets = [len(name)-x for x in splits if len(name)-x < max_chars]
        min_late_offset = (len(late_offsets) > 0) and max(late_offsets) or 0

        # If we can't find any in the first or last max_chars,
        # just choose the middle split
        if max(max_early_offset, min_late_offset) == 0:
            offset = len(splits)/2
        else:
            # Otherwise, choose the better one
            if (max_early_offset > min_late_offset):
                offset = max_early_offset
            else:
                offset = -min_late_offset

        result = break_string(name[:offset], max_chars)
        result.extend( break_string(name[offset:], max_chars) )
        return result

    # if we can't find *any* good place to split, just pick a place
    result = [name[:max_chars]]
    result.extend( break_string(name[max_chars:], max_chars) )
    return result


# Inserts line breaks into a name to make it fit nicely into a legend
def line_break_stage_name(name, max_chars = 40):
    # First adjust the input to the basic format we want.
    split = name.split(' ')
    words = [ ((len(x) <= 1) and '->' or x) for x in split]
    legend_string = ' '.join(words)

    broken_string = break_string(legend_string, max_chars)
    return '\n'.join( broken_string )


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
                stage_avg = util.time_util.string_to_microseconds(stage_match.group(2))
                stage_stddev = util.time_util.string_to_microseconds(stage_match.group(3))
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
        stage_labels.append( line_break_stage_name(stage, 30) )

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
