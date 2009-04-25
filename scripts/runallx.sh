#!/bin/sh
export DISPLAY=:0.0
export XAUTHORITY=$HOME/.Xauthority
ssh -Y -l $CLUSTERUSER meru00 $* 0 &
sleep .1
ssh -Y -l $CLUSTERUSER meru01 $* 1 &
sleep .1
ssh -Y -l $CLUSTERUSER meru02 $* 2 &
sleep .1
ssh -Y -l $CLUSTERUSER meru03 $* 3 &
sleep .1
ssh -Y -l $CLUSTERUSER meru04 $* 4 &
sleep .1
ssh -Y localhost $* 5 &
sleep .1
ssh -Y localhost $* 6 &
sleep .1
ssh -Y localhost $* 7 &
sleep .1
ssh -Y localhost $* 8 &
wait
