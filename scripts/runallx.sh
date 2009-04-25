#!/bin/sh
ssh -Y -l $CLUSTERUSER meru00 $* 0 &
ssh -Y -l $CLUSTERUSER meru01 $* 1 &
ssh -Y -l $CLUSTERUSER meru02 $* 2 &
ssh -Y -l $CLUSTERUSER meru03 $* 3 &
ssh -Y -l $CLUSTERUSER meru04 $* 4 &
ssh -Y localhost $* 5 &
ssh -Y localhost $* 6 &
ssh -Y localhost $* 7 &
ssh -Y localhost $* 8 &
wait
