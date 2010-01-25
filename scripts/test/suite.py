#!/usr/bin/python
#
# suite.py - Test suite

import sys
import os
import os.path
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

        # Make sure directory for test exists, enter it
        starting_dir = os.getcwd();
        self.__ensure_in_directory(starting_dir, name)

        try:
            test.run()
        except:
            print "Error: Caught exception in test '" + self.__get_name(test) + "':", str(sys.exc_info()[0])

        # And make sure we get out of that directory
        os.chdir(starting_dir)

        print "TEST", self.__get_name(test), "- Finished"

    def run_all(self):
        # Make sure directory for test exists, enter it
        starting_dir = os.getcwd();
        self.__ensure_in_directory(starting_dir, self.name)

        for test in self.tests:
            self.run(test.name)

        # And make sure we get out of that directory
        os.chdir(starting_dir)

    def __get_name(self, test):
        return self.name + "::" + test.name

    def __ensure_in_directory(self, start_dir, dir_offset):
        dir = os.path.join(start_dir, dir_offset)
        if not os.path.isdir(dir):
            if os.path.exists(dir):
                print "Error: Directory exists but is not directory."
                raise x
            os.mkdir(dir)
        os.chdir(dir)

if __name__ == "__main__":

    suite = TestSuite()
    suite.add( ShellCommandTest('default_sim', ['../../cluster/sim.py']) )
    suite.add( ShellCommandTest('default_packet_latency', ['../../bench/packet_latency_by_load.py', '10']) )

    if len(sys.argv) < 2:
        suite.run_all()
    else:
        for name in sys.argv[1:]:
            suite.run(name)
