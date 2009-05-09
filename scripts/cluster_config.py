#!/usr/bin/python

class ClusterConfig:
    def __init__(self):
        self.nodes = ["meru00", "meru01", "meru02", "meru03", "meru04"]
        self.user = "meru"
        self.deploy_nodes = []

    def generate_deployment(self, count, repeat = True):
        if (self.user == None or self.nodes == None or len(self.nodes) == 0):
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
            if (opt_name == "user"):
                self.user = opt_value.strip()
            elif (opt_name == "node"):
                self.nodes.append(opt_value.strip())
        fp.close()
        return


if __name__ == "__main__":
    testcc = ClusterConfig()
    print "User: ", testcc.user
    print "Nodes: ", testcc.nodes
    print "2 Deployment: ", testcc.generate_deployment(2)
    print "6 Deployment: ", testcc.generate_deployment(6)
    print "6 Deployment, No Repeat: ", testcc.generate_deployment(6, False)
