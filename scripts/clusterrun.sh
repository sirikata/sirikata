#!/bin/bash

[ "$CLUSTERUSER" ] || ( echo " NEED CLUSTERUSER env var set " && cd /tmp )

cd ../build/cmake || cd ../../build/cmake || cd build/cmake
#../../scripts/runall.sh "ls -d sirikata || (git clone git://github.com/sirikata/sirikata.git && cd sirikata && make depends && git submodule init && git submodule update && cd build/cmake && cmake -DCMAKE_INSTALL_PREFIX=/home/$CLUSTERUSER/cobbler/dependencies/installed-sirikata . && make -j2) ; echo"
#../../scripts/runall.sh "cd sirikata && git pull origin master:master && git submodule init && git submodule update && cd build/cmake && cmake -DCMAKE_INSTALL_PREFIX=/home/$CLUSTERUSER/cobbler/dependencies/installed-sirikata . && make -j2 && make install ; echo" 
#../../scripts/runall.sh "ls -d cobbler || (git clone git@ahoy:cbr.git cobbler && cd cobbler && sh install-deps ); echo "
#../../scripts/runall.sh "cd cobbler && git pull origin master:master && cd build/cmake && cmake -DBOOST_ROOT=/home/$CLUSTERUSER/sirikata/dependencies . && make -j2; echo "
export bandwidth=15000
export num=3
export numnum=9
export duration=100s
export datetime=`date +"%Y-%m-%d %H:%M:%S%z"`
DISPLAY=:0.0 ../../scripts/runallx.sh "cd cobbler/build/cmake&&python ../../scripts/runoneserversim.py $numnum --duration=$duration --bandwidth=$bandwidth --wait-additional=6 --flatness=.001 --capexcessbandwidth=false --object.queue=fairfifo --wait-until=$datetime "

echo done