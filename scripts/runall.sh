#!/bin/sh
ssh -l $CLUSTERUSER meru00 $* 0 &
ssh -l $CLUSTERUSER meru01 $* 1 &
ssh -l $CLUSTERUSER meru02 $* 2 &
ssh -l $CLUSTERUSER meru03 $* 3 &
ssh -l $CLUSTERUSER meru04 $* 4 &
wait
