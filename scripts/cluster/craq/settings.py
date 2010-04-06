#!/usr/bin/python

# The ZooKeeper Node is the hostname of the node we use for zookeeper.
CRAQ_ZOOKEEPER_NODE='meru05'
# And since naming isn't working properly and we need a specific port,
# the _ADDR is IP:Port we use to contact that node
CRAQ_ZOOKEEPER_ADDR='192.168.1.7:9888'

# These are the nodes we actually run the chain on.  Routers will connect
# to these.
CRAQ_CHAIN_NODES = ['meru27', 'meru29', 'meru30']
