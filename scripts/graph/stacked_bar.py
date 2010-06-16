#!/usr/bin/python

import sys
import random
import matplotlib.pyplot as plt
import subprocess
import colors
import fonts
import math

#indices: horizontal indices, i.e. separate experiments
#labels: horizontal labels
#vals: array of arrays of sample values. internal array is # experiments in size
#errors: array of arrays of stddev of previous values
#group_labels: labels for each layer of stacked chart
def stacked_bar(title, xlabel, ylabel, indices, labels, widths, vals, errors, group_labels):
    random.seed(0)

    num_bars = len(indices)

    legend_width = 4.5
    fig_height = 10
    fig_width = legend_width + 1 + num_bars

    legend_frac = legend_width / fig_width

    plt.figure(1, figsize=(fig_width, fig_height), dpi=600)
    plt.subplot(1,1,1)
    #plt.subplots_adjust(top=.95, bottom=.179, left=.25, right=1)
    plt.subplots_adjust(top=.95, bottom=.05, left=legend_frac, right=.99)
    sum = [0] * num_bars
    groups = []

    startList=[]
    widthList=[]

    bar_space = 1.0
    bar_offset = 0.05
    bar_width = 0.9
    for x in indices:
        startList.append((x-1)*bar_space+bar_offset);
        widthList.append((x-1)*bar_space+bar_space/2);

    for val,err,width in zip(vals,errors,widths):
        abs_val = [math.fabs(v) for v in val]

        col = colors.get_random_color()
        px = plt.bar(startList, abs_val, bottom=sum, yerr=err, color=col, width=bar_width);
        groups.append(px[0])
        sum = [pre_sum+x for pre_sum,x in zip(sum,abs_val)]

    plt.title(title)
    plt.xlabel(xlabel)
    plt.ylabel(ylabel)
    max_sum = max(sum)
    plt.xticks( widthList, labels)
    plt.ylim( (0, max_sum*1.05) )
    plt.xlim( (0, num_bars*bar_space) )

    if (len(groups) > 0):
        groups.reverse()
        group_labels.reverse()
        leg = plt.legend( groups, group_labels, loc=(-legend_width/(1+num_bars), 0) )
        fonts.set_legend_fontsize(leg, 10)

    return plt;

#do main loop through stats file
if __name__ == "__main__":

    menMeans = (20, 35, 30, 35, 27)
    womenMeans = (25, 32, 34, 20, 25)
    menStd = (2, 3, 4, 1, 2)
    womenStd = (3, 5, 2, 3, 3)
    ind = (1, 2, 3, 4, 5)    # the x locations for the groups
    labels = ('G1', 'G2', 'G3', 'G4', 'G5')
    group_labels = ('Men', 'Women')

    chart = stacked_bar('Scores', 'Group', 'Score', ind, labels, [menMeans, womenMeans], [menStd, womenStd], group_labels)
    chart.show()
