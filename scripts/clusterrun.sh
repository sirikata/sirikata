#!/bin/bash

[ "$CLUSTERUSER" ] || ( echo " NEED CLUSTERUSER env var set " && cd /tmp )

cd ../build/cmake || cd ../../build/cmake || cd build/cmake
#../../scripts/runall.sh "ls -d sirikata || (git clone git://github.com/sirikata/sirikata.git && cd sirikata && make depends && git submodule init && git submodule update && cd build/cmake && cmake -DCMAKE_INSTALL_PREFIX=/home/$CLUSTERUSER/cobbler/dependencies/installed-sirikata . && make -j2) ; echo"
#../../scripts/runall.sh "cd sirikata && git pull origin master:master && git submodule init && git submodule update && cd build/cmake && cmake -DCMAKE_INSTALL_PREFIX=/home/$CLUSTERUSER/cobbler/dependencies/installed-sirikata . && make -j2 && make install ; echo" 
#../../scripts/runall.sh "ls -d cobbler || (git clone git@ahoy:cbr.git cobbler && cd cobbler && sh install-deps ); echo "
#../../scripts/runall.sh "cd cobbler && git pull origin master:master && cd build/cmake && cmake -DBOOST_ROOT=/home/$CLUSTERUSER/sirikata/dependencies . && make -j2; echo "
export bandwidth=65536
export num=3
export numnum=9
export numobjects=256
export duration=100s
export squeue=fair
export oqueue=fairfifo
export flatness=.001

export datetime=`date +"%Y-%m-%d %H:%M:%S%z"`

DISPLAY=:0.0 ../../scripts/runallx.sh "cd cobbler/build/cmake&&python ../../scripts/runoneserversim.py $numnum --duration=$duration --send-bandwidth=$bandwidth --receive-bandwidth=$bandwidth --wait-additional=6 --flatness=$flatness --capexcessbandwidth=false --objects=$numobjects --server.queue=$squeue --object.queue=$oqueue --wait-until=$datetime "
scp $CLUSTERUSER@meru00:cobbler/build/cmake/trace-0001.txt .
scp $CLUSTERUSER@meru01:cobbler/build/cmake/trace-0002.txt .
scp $CLUSTERUSER@meru02:cobbler/build/cmake/trace-0003.txt .
scp $CLUSTERUSER@meru03:cobbler/build/cmake/trace-0004.txt .
scp $CLUSTERUSER@meru04:cobbler/build/cmake/trace-0005.txt .

scp $CLUSTERUSER@meru00:cobbler/build/cmake/sync-0001.txt .
scp $CLUSTERUSER@meru01:cobbler/build/cmake/sync-0002.txt .
scp $CLUSTERUSER@meru02:cobbler/build/cmake/sync-0003.txt .
scp $CLUSTERUSER@meru03:cobbler/build/cmake/sync-0004.txt .
scp $CLUSTERUSER@meru04:cobbler/build/cmake/sync-0005.txt .

./cbr --id=1 --layout="<$num,$num,1>" --duration=$duration --send-bandwidth=$bandwidth --receive-bandwidth=$bandwidth  --flatness=$flatness --capexcessbandwidth=false --object.queue=fairfifo --serverips=serverip-$num-$num.txt --analysis.windowed-bandwidth=packet  --analysis.windowed-bandwidth.rate=100ms
./cbr --id=1 --layout="<$num,$num,1>" --duration=$duration --send-bandwidth=$bandwidth --receive-bandwidth=$bandwidth  --flatness=$flatness --capexcessbandwidth=false --object.queue=fairfifo --serverips=serverip-$num-$num.txt --analysis.windowed-bandwidth=datagram --analysis.windowed-bandwidth.rate=100ms
python ../../scripts/graph_windowed_bandwidth.py windowed_bandwidth_packet_send.dat &
python ../../scripts/graph_windowed_bandwidth.py windowed_bandwidth_packet_receive.dat &
python ../../scripts/graph_windowed_bandwidth.py windowed_bandwidth_datagram_send.dat &
python ../../scripts/graph_windowed_bandwidth.py windowed_bandwidth_datagram_receive.dat &
wait
export datadir="d-$numnum-$bandwidth-$flatness-$oqueue-$numobjects-$squeue-"`date +%H-%M-%S-%Y%m%d`
mkdir $datadir
mv trace-* $datadir
mv sync-* $datadir
mv windowed_bandwidth* $datadir

echo done