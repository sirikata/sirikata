#!/bin/bash

export bandwidth=100000
export num=3
export max_servers=10

python ../../scripts/dynamic_svrmap_runsim.py $max_servers --duration=100s --receive-bandwidth=$bandwidth --send-bandwidth=$bandwidth --wait-additional=6 --flatness=.001 --capexcessbandwidth=false --object.queue=fairfifo --max-servers=$max_servers "--layout=<$num,$num,1>"

./cbr --id=1 "--layout=<$num,$num,1>" --serverips=serverip-$num-$num.txt --duration 100s --analysis.locvis=1  --max-servers=$max_servers

./cbr "--layout=<$num,$num,1>" --serverips=serverip-$num-$num.txt --duration 100s --analysis.windowed-bandwidth packet --max-servers=$max_servers

python ../../scripts/graph_windowed_bandwidth.py windowed_bandwidth_packet_send.dat

