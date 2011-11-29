import numpy
import sys
import os
from scipy import stats

def show_data():
    mean_vals = []
    for i in range(0,101,10):
        data = numpy.fromfile('results-%d.txt' % i, sep=' ')
        
        print len(data), numpy.sum(data), numpy.mean(data), \
                numpy.median(data), stats.scoreatpercentile(data, 90), \
                stats.scoreatpercentile(data, 10)

def main():
    if len(sys.argv) != 2:
        print >> sys.stderr, 'Usage: python meshtool_aggregate.py dir'
        sys.exit(1)
    
    subdirs = os.listdir(sys.argv[1])

#    for meshdir in subdirs:
#        print meshdir[:30].ljust(30), '\t',
#        for level in range(11):
#            res_file = os.path.join(sys.argv[1], meshdir, 'results-%d.txt' % level)
#            if not os.path.isfile(res_file):
#                continue
#            data = numpy.fromfile(res_file, sep=' ')
#            print '%06.2f' % numpy.mean(data),
#        print

    for level in range(11):
        level_data = []
        for meshdir in subdirs:
            res_file = os.path.join(sys.argv[1], meshdir, 'results-%d.txt' % level)
            if not os.path.isfile(res_file):
                continue
            data = numpy.fromfile(res_file, sep=' ')
            level_data.append(data)
            
        data = numpy.concatenate(level_data)
        
        print len(data), numpy.sum(data), numpy.mean(data), \
                numpy.median(data), stats.scoreatpercentile(data, 90), \
                stats.scoreatpercentile(data, 10)

if __name__ == '__main__':
    main()