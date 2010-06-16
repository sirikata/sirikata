#!/usr/bin/python

#benchmark.py - Runs benchmark scripts across a cluster of nodes and collects
#the output.  Arguments are passed directly to cbr_wrapper.sh and the underlying
#bench binary.

import sys

CBR_WRAPPER = "util/cbr_wrapper.sh"

from config import ClusterConfig
from run import ClusterSubstitute,ClusterRun,ClusterDeploymentRun
from scp import ClusterSCP

def scripts_dir(cc):
    return cc.code_dir + "/scripts"

if __name__ == "__main__":

    bench_args = " ".join(sys.argv[1:])

    cc = ClusterConfig()

    clean_cmd = "cd " + scripts_dir(cc) + "; " + CBR_WRAPPER + " bench " + bench_args + ";"
    ClusterRun(cc, clean_cmd)
