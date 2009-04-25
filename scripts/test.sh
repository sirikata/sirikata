#!/bin/bash
#for ((bandwidth = 10000; bandwidth < 105000; bandwidth = bandwidth + 10000))
#do
export bandwidth=15000
export num=3
export numnum=9
export duration=100s
cd ../build/cmake
cd ../../build/cmake
cd build/cmake
python ../../scripts/runsim.py $numnum --duration=$duration --bandwidth=$bandwidth --wait-additional=6 --flatness=.001 --capexcessbandwidth=false --object.queue=fairfifo
#    ./cbr --id=1 --serverips=serverip-$num-$num.txt --duration $duration --analysis.loc true >> loc_error.txt
 ./cbr --id=1 "--layout=<$num,$num,1>" --serverips=serverip-$num-$num.txt --duration $duration --analysis.locvis=1 &
./cbr --id=1 "--layout=<$num,$num,1>" --serverips=serverip-$num-$num.txt --duration $duration --analysis.bandwidth true
./cbr --id=1 "--layout=<$num,$num,1>" --serverips=serverip-$num-$num.txt --duration $duration --analysis.windowed-bandwidth datagram
./cbr --id=1 "--layout=<$num,$num,1>" --serverips=serverip-$num-$num.txt --duration $duration --analysis.windowed-bandwidth packet
#done
