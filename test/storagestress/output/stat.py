#!/usr/bin/python
import os
import sys
from histplot import *
from cdfplot import *
from numpy import *
from pylab import *

if len (sys.argv) not in [4, 5]:
    print 'Usage: stats <output-file-name> <storage(Cassandra/SQLite)> <graph-type(hist/cdf)> <bins> <1/0: x-axis consistency>'
    sys.exit(0)

file = open(sys.argv[1], 'r')
lines = file.readlines()

name = sys.argv[2]
type = sys.argv[3]
bins = int(sys.argv[4])

xbool = 0
if len(sys.argv) == 6:
  xbool = int(sys.argv[5])

singleWritesTime = []
singleReadsTime = []
singleErasesTime = []
batchWritesTime = []
batchReadsTime = []
batchErasesTime = []

swt_mean = 0
srt_mean = 0
set_mean = 0
bwt_mean = 0
brt_mean = 0
bet_mean = 0

s_min = 0
s_max = 0
b_min = 0
b_max = 0

swt_std = 0
srt_std = 0
set_std = 0
bwt_std = 0
brt_std = 0
bet_std = 0

dataLength = 0
keyNum = 0
bucketNum = 0
rounds = 0
operationNum = 0

for line in lines:                                                                                                                     
    record = line.split()
    if len(record) > 0:
        if record[0] == 'dataLength:':
            dataLength = record[1].strip(',')
            keyNum = int(record[3].strip(','))
            bucketNum = int(record[5].strip(','))
            rounds = int(record[7].strip(','))
            operationNum = keyNum*bucketNum
            #print dataLength, keyNum, bucketNum, rounds, operationNum
        elif record[0] == 'testSingleWrites':
            singleWritesTime.append(float(record[4])/operationNum)
        elif record[0] == 'testSingleReads':
            singleReadsTime.append(float(record[4])/operationNum)
        elif record[0] == 'testSingleErases':
            singleErasesTime.append(float(record[4])/operationNum)
        elif record[0] == 'testBatchWrites':
            batchWritesTime.append(float(record[4])/operationNum)
        elif record[0] == 'testBatchReads':
            batchReadsTime.append(float(record[4])/operationNum)
        elif record[0] == 'testBatchErases':
            batchErasesTime.append(float(record[4])/operationNum)

_swt = array(singleWritesTime)
_srt = array(singleReadsTime)
_set = array(singleErasesTime)
_bwt = array(batchWritesTime)
_brt = array(batchReadsTime)
_bet = array(batchErasesTime)

swt_mean = _swt.mean()
srt_mean = _srt.mean()
set_mean = _set.mean()
bwt_mean = _bwt.mean()
brt_mean = _brt.mean()
bet_mean = _bet.mean()

swt_std = _swt.std()
srt_std = _srt.std()
set_std = _set.std()
bwt_std = _bwt.std()
brt_std = _brt.std()
bet_std = _bet.std()

s_min = min(min(_swt), min(_srt), min(_set))
s_max = max(max(_swt), max(_srt), max(_set))
b_min = min(min(_bwt), min(_brt), min(_bet))
b_max = max(max(_bwt), max(_brt), max(_bet))

#print swt_mean, srt_mean, set_mean, bwt_mean, brt_mean, bet_mean
#print swt_std, srt_std, set_std, bwt_std, brt_std, bet_std

settings = '\nbuckets number = %d, keys per bucket = %d, data size = %s bytes, sample size = %d'%(bucketNum,keyNum,dataLength,rounds)

if type == 'hist':

    fig = figure() 
    fig.suptitle('%s - Histogram (bins = %d) \n%s'%(name, bins, settings))

    ax = fig.add_subplot(321)
    histplot(ax, _swt, bins, swt_mean, swt_std, s_min, s_max, 'single writes time', xbool)
    ax = fig.add_subplot(323)
    histplot(ax, _srt, bins, srt_mean, srt_std, s_min, s_max, 'single reads time', xbool)
    ax = fig.add_subplot(325)
    histplot(ax, _set, bins, set_mean, set_std, s_min, s_max, 'single erases time', xbool)
    ax = fig.add_subplot(322)
    histplot(ax, _bwt, bins, bwt_mean, bwt_std, b_min, b_max, 'batch writes time', xbool)
    ax = fig.add_subplot(324)
    histplot(ax, _brt, bins, brt_mean, brt_std, b_min, b_max, 'batch reads time', xbool)
    ax = fig.add_subplot(326)
    histplot(ax, _bet, bins, bet_mean, bet_std, b_min, b_max, 'batch erases time', xbool)

    show()

elif type == 'cdf':
    
    fig = figure()
    fig.suptitle('%s Tests - CDF\n%s'%(name, settings))

    ax = fig.add_subplot(321)
    cdfplot(ax, _swt, bins, swt_mean, swt_std, s_min, s_max, 'single writes time', xbool)
    ax = fig.add_subplot(323)
    cdfplot(ax, _srt, bins, srt_mean, srt_std, s_min, s_max, 'single reads time', xbool)
    ax = fig.add_subplot(325)
    cdfplot(ax, _set, bins, set_mean, set_std, s_min, s_max, 'single erases time', xbool)
  
    ax = fig.add_subplot(322)
    cdfplot(ax, _bwt, bins, bwt_mean, bwt_std, b_min, b_max, 'batch writes time', xbool)
    ax = fig.add_subplot(324)
    cdfplot(ax, _brt, bins, brt_mean, brt_std, b_min, b_max, 'batch reads time', xbool)
    ax = fig.add_subplot(326)
    cdfplot(ax, _bet, bins, bet_mean, bet_std, b_min, b_max, 'batch erases time', xbool)

    show()






