#!/bin/bash
#for ((bandwidth = 10000; bandwidth < 105000; bandwidth = bandwidth + 10000))
#do
export bandwidth=100000
export num=3
export numnum=9
python ../../scripts/dynamic_svrmap_runsim.py $numnum --duration=100s --receive-bandwidth=$bandwidth --send-bandwidth=$bandwidth --wait-additional=6 --flatness=.001 --capexcessbandwidth=false --object.queue=fairfifo
#./cbr --id=1 --serverips=serverip-3-3.txt --duration 100s --analysis.loc true >> loc_error.txt
./cbr --id=1 "--layout=<$num,$num,1>" --serverips=serverip-$num-$num.txt --duration 100s --analysis.locvis=1 #&> windowed_bandwidth.txt

./cbr  "--layout=<4,4,1>" --serverips=serverip-$num-$num.txt --duration 100s --analysis.windowed-bandwidth packet

python ../../scripts/graph_windowed_bandwidth.py windowed_bandwidth_packet_send.dat

#done
