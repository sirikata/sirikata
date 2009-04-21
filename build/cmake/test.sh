#!/bin/bash
#for ((bandwidth = 10000; bandwidth < 105000; bandwidth = bandwidth + 10000))
#do
export bandwidth=100000
export num=3
export numnum=9
python ../../scripts/runsim.py $numnum --duration=100s --bandwidth=$bandwidth --wait-additional=6 --flatness=.001 --capexcessbandwidth=false --object.queue=fairfifo
#    ./cbr --id=1 --serverips=serverip-3-3.txt --duration 100s --analysis.loc true >> loc_error.txt
./cbr --id=1 "--layout=<$num,$num,1>" --serverips=serverip-$num-$num.txt --duration 100s --analysis.locvis=1 #&> windowed_bandwidth.txt
#    ./cbr --id=1 "--layout=<2,2,1>" --serverips=serverip-3-3.txt --duration 100s --analysis.bandwidth true
#done
