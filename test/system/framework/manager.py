#!/usr/bin/python

from __future__ import print_function
import sys, os, time, types
import shutil
import os.path
from datetime import datetime

from tests.test import Test

_this_script_dir = os.path.dirname(__file__)

DEFAULT_OUTPUT_FILENAME = 'unitTestResults.txt';
DEFAULT_BIN_PATH = os.path.normpath(os.path.join(_this_script_dir, '../../../build/cmake/'))
DEFAULT_CPPOH_BIN_NAME = 'cppoh_d'
DEFAULT_SPACE_BIN_NAME = 'space_d'

DEFAULT_OUTPUT_FOLDER = 'output'

class Manager:
    def __init__(self):
        # Maintain an array and a dict -- the dict for easy lookup,
        # the array to allow us to keep things ordered so tests can
        # run in the sequence we want
        self._tests = [];
        self._testsByName = {}

    def add(self, test):
        self._tests.append(test);
        self._testsByName[test.name] = testToAdd

    def collect(self, searchmod):
        """
        Search the given module for Test subclasses, collect them,
        order them according to dependencies, and add them to this
        Manager to be instantiated and executed.
        """
        # Find all subclasses of Test
        all_test_classes = set()
        modules = [searchmod]
        considered = set()
        while len(modules):
            mod = modules[0]
            modules = modules[1:]
            for child_name in dir(mod):
                child = getattr(mod, child_name)
                if type(child) == types.ModuleType and child not in considered:
                    modules.append(child)
                    considered.add(child)
                elif type(child) == types.TypeType:
                    if issubclass(child, Test):
                        all_test_classes.add(child)

        # Remove non-leaf classes, e.g. if you have a helper subclass
        # before you hit the actual implementation of the test, remove
        # these
        leaf_test_classes = set(all_test_classes)
        for test_class in all_test_classes:
            for base_class in test_class.__bases__:
                if base_class in leaf_test_classes: leaf_test_classes.remove(base_class)

        # Remove any tests marked as disabled
        leaf_test_classes = set([x for x in leaf_test_classes if not x.disabled])

        # Use ordering requests to generate an order for tests using
        # simple dependency analysis. By using no_deps_left as a
        # stack, we get a depth first traversal so related tests are
        # (hopefully) near each other. By sorting the lists, we add
        # some predictability to the output ordering instead of
        # relying on the order we get out of set traversal when
        # generating the lists.
        no_deps_left = sorted([x for x in leaf_test_classes if not x.after], key=lambda y: y.__module__ + y.__name__)
        deps_remain = sorted([x for x in leaf_test_classes if x.after], key=lambda y: y.__module__ + y.__name__)
        test_classes = []
        while len(no_deps_left) > 0:
            klass = no_deps_left.pop()
            test_classes.append(klass)
            # Add to no_deps_left if length of deps not already in used is 0
            no_deps_left = no_deps_left + [x for x in deps_remain if len([True for dep in x.after if dep not in test_classes]) == 0]
            # Similarly, filter those items out of deps_remain
            deps_remain = [x for x in deps_remain if len([True for dep in x.after if dep not in test_classes]) > 0]

        if len(deps_remain) != 0:
            raise RuntimeError("Tests with ordering dependencies remain, but none are candidates to be used next. Check the settings of 'after' in your test classes.")

        self._tests = test_classes
        self._testsByName = dict([(t.name, t) for t in self._tests])


    def run(self, testNames=None, output=sys.stdout, binPath=DEFAULT_BIN_PATH, cppohBinName=DEFAULT_CPPOH_BIN_NAME, spaceBinName=DEFAULT_SPACE_BIN_NAME, saveOutput=False):

        if not testNames:
            testNames = [t.name for t in self._tests]
        numTests = len(testNames);
        count = 1;

        suite_start_time = datetime.now()

        #if the dirty folder already exists then delete it first
        if(os.path.exists(DEFAULT_OUTPUT_FOLDER)):
            shutil.rmtree(DEFAULT_OUTPUT_FOLDER);

        succeeded = 0
        failed = 0
        for s in testNames:
            if s in self._testsByName:
                test = self._testsByName[s];
            else:
                # Try to find the test with some fuzzy matching
                found = [x for x in self._tests if x.name.lower().find(s.lower()) != -1]
                if len(found) == 0:
                    raise RuntimeError("Couldn't find test matching " + s)
                elif len(found) > 1:
                    raise RuntimeError("Found too many tests matching " + s)
                else:
                    test = found[0]
                    s = test.name

            start_time = datetime.now()
            print("*" * 40, file=output)
            print('BEGIN TEST', s, '(' + str(count) + ' of ' + str(numTests) + ')', file=output)

            folderName = os.path.join(DEFAULT_OUTPUT_FOLDER, s);
            if (os.path.exists(folderName)):
                print('WARNING: deleting previous output file at ' + folderName, file=output);
                shutil.rmtree(folderName);

            os.makedirs(folderName);
            test_inst = test()
            test_inst.setOutput(output)
            test_inst.runTest(folderName,binPath=binPath, cppohBinName=cppohBinName, spaceBinName=spaceBinName, output=output)

            print("TEST", (test_inst.failed and "FAILED" or "PASSED"), file=output)
            if test_inst.failed:
                failed += 1
                test_inst.report()
            else:
                succeeded += 1

            finish_time = datetime.now()
            elapsed = finish_time - start_time
            elapsed_str = str(elapsed.seconds + (float(elapsed.microseconds)/1000000.0)) + 's'

            print('END TEST', elapsed_str, '(' + str(count) + ' of ' + str(numTests) + ')', file=output)
            print("*" * 40, file=output)
            print(file=output)

            count += 1;


        suite_finish_time = datetime.now()
        elapsed = suite_finish_time - suite_start_time
        elapsed_str = str(elapsed.seconds + (float(elapsed.microseconds)/1000000.0)) + 's'

        print(file=output)
        print("*" * 40, file=output)
        print("SUMMARY:", succeeded, "succeeded,", failed, "failed, in", elapsed_str, file=output)
        print("*" * 40, file=output)

        if (not saveOutput) and os.path.exists(DEFAULT_OUTPUT_FOLDER):
            shutil.rmtree(DEFAULT_OUTPUT_FOLDER);
