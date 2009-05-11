#!/usr/bin/python

import os.path

class ClusterNode:
    def __init__(self, spec_string):
        (self.user, self.node) = spec_string.split('@',1)

    def str(self):
        return self.user + "@" + self.node

class ClusterConfig:
    def __init__(self):
        default_path = os.path.expanduser("~/.cluster")
        self.nodes = []
        self.deploy_nodes = []
        self.code_dir = "cbr"
        self.port_base = 6666
        if (os.path.exists(default_path)):
            self.parse(default_path)
        else: # just setup some default nodes that make sense for us
            self.nodes.append( ClusterNode("meru@meru00") )
            self.nodes.append( ClusterNode("meru@meru01") )
            self.nodes.append( ClusterNode("meru@meru02") )
            self.nodes.append( ClusterNode("meru@meru03") )
            self.nodes.append( ClusterNode("meru@meru04") )

    def generate_deployment(self, count, repeat = True):
        if (self.nodes == None or len(self.nodes) == 0):
            return None
        self.deploy_nodes = []
        node_idx = 0
        while( len(self.deploy_nodes) < count ):
            if (node_idx >= len(self.nodes)):
                if (repeat == False):
                    return None
                node_idx = 0
            self.deploy_nodes.append(self.nodes[node_idx])
            node_idx = node_idx + 1
        return self.deploy_nodes

    # Parse options from the given file
    def parse(self, filename):
        fp = open(filename, 'r')
        if (fp == None):
            return
        # clear out any old values
        self.user = None
        self.nodes = []
        # line format is option = value
        for line in fp:
            [opt_name, opt_value] = line.split('=', 1)
            opt_name = opt_name.strip()
            if (opt_name == "node"):
                self.nodes.append( ClusterNode(opt_value.strip()) )
            elif (opt_name == "code_dir"):
                self.code_dir = opt_value.strip()
            elif (opt_name == "port"):
                self.port_base = int(opt_value)
        fp.close()
        return


if __name__ == "__main__":
    testcc = ClusterConfig()
    print "Nodes: ", testcc.nodes
    print "Code Directory: ", testcc.code_dir
    print "2 Deployment: ", testcc.generate_deployment(2)
    print "6 Deployment: ", testcc.generate_deployment(6)
    print "6 Deployment, No Repeat: ", testcc.generate_deployment(6, False)
