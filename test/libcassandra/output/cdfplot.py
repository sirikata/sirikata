from numpy import *
from pylab import *
import matplotlib.cm as cm
import matplotlib.colors as colors

def cdfplot(ax, data, binN, mean, std, _min, _max, label_name, xbool):

  N, bins, patches = ax.hist(data, binN, cumulative=True, normed=True,  histtype='step')

  xlabel_str = label_name+' (ms)\nmean = %.3f, std = %.3f'%(mean, std)
  xlabel(xlabel_str)
  ylabel("count")
  
  if xbool == 1:
    xlim(_min, _max)
  ylim(0, 1)



