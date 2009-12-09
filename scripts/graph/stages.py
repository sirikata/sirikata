#!/usr/bin/python

import sys
import re
import matplotlib.pyplot as plt

# FIXME It would be nice to have a better way of making this script able to find
# other modules in sibling packages
sys.path.insert(0, sys.path[0]+"/..")

import util.time_util

# For a set of log files listing individual samples for various (stage,stage) pairs,
# produce 1 graph per trial x (stage,stage) showing all sample durations, using
# the specified output filename as a prefix, which will have the stage names appended,
# e.g. logfile -> files[logfile] + '.' + (stage,stage) + '.pdf'
#
# files - dict of input log files to output graph prefixes
def graph_stages_raw_samples(files):
    for (infile,out_template) in files:
        sample_pattern = '^([^ -]*) - ([^ -]*) (.*s)'

        fp = open(infile, 'r')

        # data maps (start_tag, end_tag) => [samples]
        data = {}

        # Read in all the samples, organizing by stage pairs
        for line in fp:
            match = re.search(sample_pattern, line)
            if match == None:
                continue

            start_tag = match.group(1)
            end_tag = match.group(2)
            tag_pair = (start_tag, end_tag)

            diff_str = match.group(3)
            diff = util.time_util.string_to_microseconds(diff_str)

            if not tag_pair in data:
                data[tag_pair] = []

            data[tag_pair].append(diff)

        fp.close()


        # Now, generate a graph for each key, i.e. each stage pair
        for tag_pair, diff_array in data.items():
            start_tag, end_tag = tag_pair

            fig = plt.figure()
            ax = fig.add_subplot(111)

            n, bins, patches = ax.hist(diff_array, 100, facecolor='green', alpha=0.75)

            bincenters = 0.5*(bins[1:]+bins[:-1])

            ax.set_xlabel('Duration (us)')
            ax.set_ylabel('Sample Count')
            ax.set_title('Histogram of Stage Latencies');
            ax.grid(True)

            output_file = '.'.join([out_template, start_tag, end_tag, 'pdf'])
            plt.savefig(output_file)
