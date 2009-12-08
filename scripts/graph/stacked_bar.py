#!/usr/bin/python

import sys
import random
import matplotlib.pyplot as plt
import subprocess
import colors
import fonts

#indices: horizontal indices, i.e. separate experiments
#labels: horizontal labels
#vals: array of arrays of sample values. internal array is # experiments in size
#errors: array of arrays of stddev of previous values
#group_labels: labels for each layer of stacked chart
def stacked_bar(title, xlabel, ylabel, indices, labels, widths, vals, errors, group_labels):
    random.seed(0)

    plt.figure(1, figsize=(30,15), dpi=600)
    plt.subplot(1,1,1)
    plt.subplots_adjust(wspace=.5, top=.95, bottom=.179, left=.25, right=1)
    sum = [0] * len(indices)
    groups = []
    max_width=0;
    for widlist in widths:
        for wid in widlist:
            if (wid>max_width):
                max_width=wid;
    startList=[]
    widthList=[]
    for x in indices:
        startList.append((x-1)*max_width);
        widthList.append((x-1)*max_width+max_width/2);

    for val,err,width in zip(vals,errors,widths):
        col = colors.get_random_color()
        px = plt.bar(startList, val, bottom=sum, yerr=err, color=col, width=width);
        groups.append(px[0])
        sum = [pre_sum+x for pre_sum,x in zip(sum,val)]

    plt.title(title)
    plt.xlabel(xlabel)
    plt.ylabel(ylabel)
    max_sum = max(sum)
    plt.xticks( widthList, labels)
    plt.ylim( (0, max_sum*1.05) )
    plt.xlim( (0,len(widthList)*max_width*1.05) )

    if (len(groups) > 0):
        groups.reverse()
        group_labels.reverse()
        leg = plt.legend( groups, group_labels, loc=(-.333,0) )
        fonts.set_legend_fontsize(leg, 8)

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
