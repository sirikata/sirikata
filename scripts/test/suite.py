#!/usr/bin/python
#
# suite.py - Test suite

import sys
from test import Test, ShellCommandTest

class TestSuite:
    def __init__(self, name='Default'):
        self.name = name
        self.tests = []
        self.tests_by_name = {}

    def add(self, test):
        self.tests.append(test)
        self.tests_by_name[test.name] = test

    def run(self, name):
        test = self.tests_by_name[name]
        print "TEST", self.__get_name(test), "- Started"
        try:
            test.run()
        except:
            print "Error: Caught exception in test '" + self.__get_name(test) + "':", sys.exc_info()[0]
        print "TEST", self.__get_name(test), "- Finished"

    def run_all(self):
        for test in self.tests:
            self.run(test.name)

    def __get_name(self, test):
        return self.name + "::" + test.name

if __name__ == "__main__":

    suite = TestSuite()
    suite.add( ShellCommandTest('default_sim', ['cluster/sim.py']) )
    suite.add( ShellCommandTest('default_packet_latency', ['bench/packet_latency_by_load.py', '10']) )

    if len(sys.argv) < 2:
        suite.run_all()
    else:
        for name in sys.argv[1:]:
            suite.run(name)
