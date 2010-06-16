#!/usr/bin/python

import pydot
import re

if __name__ == "__main__":

    edges = {}
    same_rank_groups = []
    node_attrs = {}

    fp = open('../src/analysis/MessageLatency.cpp', 'r')

    edge_pattern = '.*addEdge\(Trace::(.*), Trace::([^,]*)(, PacketStageGraph::ASYNC)?\);[^/]*(//(.*))?'
    for line in fp:
        edge_match = re.search(edge_pattern, line)
        if edge_match:
            print edge_match.group(1), edge_match.group(2), edge_match.group(3), edge_match.group(5)
            edge_start = edge_match.group(1)
            edge_end = edge_match.group(2)
            is_async = (edge_match.group(3) != None)
            opts = edge_match.group(5)

            edge_key = (edge_start, edge_end)
            if edge_key in edges:
                raise 'Duplicate edge found: %s'%(str(edge_key))

            edges[edge_key] = {}
            if (is_async):
                edges[edge_key]['color'] = 'red'
            if (opts and opts.find('drop') != -1):
                same_rank_groups.append( [edge_start, edge_end] )
                if (edge_end not in node_attrs):
                    node_attrs[edge_end] = {}
                node_attrs[edge_end]['label'] = 'DROPPED'
    fp.close()


    # Generate the graph from the edge set, setting attributes along the way
    graph = pydot.Dot(graph_type='digraph')
    for edge, attributes in edges.items():
        e = pydot.Edge( edge[0], edge[1] )
        for attr,val in attributes.items():
            e.set(attr,val)
        graph.add_edge(e)

    # For each same rank group, generate a subgraph marked with rank=same
    for nodes in same_rank_groups:
        sg = pydot.Subgraph()
        for node in nodes:
            n = graph.get_node(node)
            if (n == None):
                raise 'No node named %s in graph'%(node)
            sg.add_node(n)
        #sg.set('rank', 'min')
        graph.add_subgraph(sg)

    # Set any individual node properties
    for node,attrs in node_attrs.items():
        n = graph.get_node(node)
        for attr,val in attrs.items():
            n.set(attr,val)

    # Save the graph
    graph.write_png('packet_flow.png', prog='dot')
