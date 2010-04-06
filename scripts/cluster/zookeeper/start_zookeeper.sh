#!/bin/bash

if [ `hostname` != "meru05" ]; then
    exit 0
fi

# FIXME It would be much better if we didn't depend on Behram's layout
rm -r /home/meru/bmistree/tmp/zookeeper_2/version-2
cd /home/meru/bmistree/zookeeper-3.1.1_2/
java -cp zookeeper-3.1.1.jar:lib/log4j-1.2.15.jar:conf org.apache.zookeeper.server.quorum.QuorumPeerMain conf/zoo.cfg
