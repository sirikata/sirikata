#!/usr/bin/env python

import sys, types
from pyTests.testManager import TestManager
from pyTests.singleTest import SingleTest

class TestRunner(object):
    """
    Manages searching for and running tests.
    """

    def __init__(self, searchmod):
        """
        Initialize the TestRunner, searching the given module for Test
        subclasses which should be instantiated and executed.
        """
        self.manager = TestManager()

        # Find all subclasses of SingleTest
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
                    if issubclass(child, SingleTest):
                        all_test_classes.add(child)
        # Remove non-leaf classes, e.g. if you have a helper subclass
        # before you hit the actual implementation of the test, remove
        # these
        leaf_test_classes = set(all_test_classes)
        for test_class in all_test_classes:
            for base_class in test_class.__bases__:
                if base_class in leaf_test_classes: leaf_test_classes.remove(base_class)

        # Use ordering requests to generate an order for tests using
        # simple dependency analysis. By using no_deps_left as a
        # stack, we get a depth first traversal so related tests are
        # (hopefully) near each other.
        no_deps_left = [x for x in leaf_test_classes if not x.after]
        deps_remain = [x for x in leaf_test_classes if x.after]
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

        self.manager.addTestArray([tc() for tc in test_classes])

    def runSome(self, testNames, output=sys.stdout, saveOutput=False):
        self.manager.runSomeTests(testNames, output=output, saveOutput=saveOutput)

    def runAll(self, output=sys.stdout, saveOutput=False):
        self.manager.runAllTests(output=output, saveOutput=saveOutput)
