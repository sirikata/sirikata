#!/usr/bin/python

import sys
import random
import matplotlib.pyplot as plt

def get_random_color():
    return (random.random(), random.random(), random.random())

def set_legend_fontsize(legend, fontsize):
    for t in legend.get_texts():
        t.set_fontsize(fontsize)
        t.set_fontname('serif')


#indices: horizontal indices, i.e. separate experiments
#labels: horizontal labels
#vals: array of arrays of sample values. internal array is # experiments in size
#errors: array of arrays of stddev of previous values
#group_labels: labels for each layer of stacked chart
def stacked_bar(title, xlabel, ylabel, indices, labels, vals, errors, group_labels):
    random.seed(0)

    width = 1.00       # the width of the bars: can also be len(x) sequence

    plt.figure(1, figsize=(30,15), dpi=600)

    sum = [0] * len(indices)
    groups = []
    for val,err in zip(vals,errors):
        col = get_random_color()
        px = plt.bar(indices, val, bottom=sum, yerr=err, color=col, width=width);
        groups.append(px[0])
        sum = [pre_sum+x for pre_sum,x in zip(sum,val)]

    plt.title(title)
    plt.xlabel(xlabel)
    plt.ylabel(ylabel)

    plt.xticks( [x + width/2. for x in indices], labels)
    max_sum = max(sum)
    plt.ylim( (0, max_sum*1.05) )

    leg = plt.legend( groups, group_labels, loc=(0,0) )
    set_legend_fontsize(leg, 8)

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
