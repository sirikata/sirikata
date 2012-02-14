from numpy import *
from pylab import *
import matplotlib.cm as cm
import matplotlib.colors as colors

def histplot(ax, data, binN, mean, std, _min, _max, label, xbool):
  N, bins, patches = ax.hist(data, binN, color='blue')

  xlabel_str = label+' (ms)\n mean = %.3f, std = %.3f'%(mean, std)
  xlabel(xlabel_str)
  ylabel("count")

  color = '0.75'
  
  if xbool == 1:
    xlim(_min, _max)


