#!/usr/bin/python
#
# suite.py - Test suite

import sys
import os
import os.path
import traceback
import datetime
from test import Test, ShellCommandTest
from sim_test import ClusterSimTest, PacketLatencyByLoadTest

import sys
# FIXME It would be nice to have a better way of making this script able to find
# other modules in sibling packages
sys.path.insert(0, sys.path[0]+"/..")

import util.stdio

class TestSuite:
    def __init__(self, name='Default'):
        self.name = name
        self.tests = []
        self.tests_by_name = {}

    def add(self, test):
        self.tests.append(test)
        self.tests_by_name[test.name] = test

    def clean(self):
        starting_dir = os.getcwd();
        self.__remove_directory(starting_dir, self.name)

    def run(self, name, io):
        test = self.tests_by_name[name]
        print >>io.stdout, "TEST", self.__get_name(test), "...",
        io.stdout.flush()

        # Make sure directory for test exists, enter it
        starting_dir = os.getcwd();
        self.__ensure_in_directory(starting_dir, name)

        success = True
        try:
            success = test.pre_run(io)
            if success:
                success = test.run(io)
                post_success = test.post_run(io)
                success = success and post_success
        except:
            if (io.stderr):
                traceback.print_exc(100, io.stderr)
            success = False

        if success:
            print >>io.stdout, " Succeeded"
        else:
            print >>io.stdout, " Failed"
        # And make sure we get out of that directory
        os.chdir(starting_dir)

    def run_all(self, io):
        # Make sure directory for test exists, enter it
        starting_dir = os.getcwd();
        self.__ensure_in_directory(starting_dir, self.name)

        for test in self.tests:
            self.run(test.name, io=io)

        # And make sure we get out of that directory
        os.chdir(starting_dir)

    def archive(self):
        dir = self.name
        assert( os.path.exists(dir) and os.path.isdir(dir) )
        timestamp = datetime.datetime.now().strftime('%Y.%m.%d.%H.%M')
        os.system('tar -c %s | bzip2 -z -9 > %s.%s.tar.bz2' % (dir, self.name, timestamp))

    def __get_name(self, test):
        return self.name + "::" + test.name

    def __remove_directory(self, start_dir, dir_offset):
        dir = os.path.join(start_dir, dir_offset)
        if not os.path.exists(dir): return

        assert os.path.isdir(dir)

        # This is fairly dangerous, add some sanity asserts:
        # hopefully not / or anything too close to /
        assert (len(dir) > 10)
        # Either path doesn't start at root or is relatively deep
        assert (not dir.startswith('/') or dir.count('/') > 3)

        os.system('rm -rf %s' % (dir))
        assert not os.path.exists(dir)

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
    #suite.add( ClusterSimTest('default_sim') )
    suite.add( PacketLatencyByLoadTest('default_packet_latency', 10) )

    packet_latency_with_caching_settings = {'duration' : '300s', 'oseg_cache_entry_lifetime' : '300s', 'num_random_objects': 50}

    # Local messages only
    suite.add( PacketLatencyByLoadTest('packet_latency_with_caching_local_4', 10, local_pings=True, remote_pings=False, settings=packet_latency_with_caching_settings, space_layout=(4,1), time_limit=datetime.timedelta(minutes=10) ) )
    suite.add( PacketLatencyByLoadTest('packet_latency_with_caching_local_8', 10, local_pings=True, remote_pings=False, settings=packet_latency_with_caching_settings, space_layout=(8,1), time_limit=datetime.timedelta(minutes=10) ) )
    # Remote messages only
    suite.add( PacketLatencyByLoadTest('packet_latency_with_caching_remote_4', 10, local_pings=False, remote_pings=True, settings=packet_latency_with_caching_settings, space_layout=(4,1), time_limit=datetime.timedelta(minutes=10) ) )
    suite.add( PacketLatencyByLoadTest('packet_latency_with_caching_remote_8', 10, local_pings=False, remote_pings=True, settings=packet_latency_with_caching_settings, space_layout=(8,1), time_limit=datetime.timedelta(minutes=10) ) )
    # Mix of messages
    suite.add( PacketLatencyByLoadTest('packet_latency_with_caching_mixed_4', 10, local_pings=True, remote_pings=True, settings=packet_latency_with_caching_settings, space_layout=(4,1), time_limit=datetime.timedelta(minutes=10) ) )
    suite.add( PacketLatencyByLoadTest('packet_latency_with_caching_mixed_8', 10, local_pings=True, remote_pings=True, settings=packet_latency_with_caching_settings, space_layout=(8,1), time_limit=datetime.timedelta(minutes=10) ) )


    if len(sys.argv) < 2:
        suite.clean()
        suite.run_all(util.stdio.StdIO())
    else:
        for name in sys.argv[1:]:
            suite.run(name)
