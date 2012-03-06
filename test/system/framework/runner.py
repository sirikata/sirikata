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

        self.manager.addTestArray([tc() for tc in leaf_test_classes])

    def runSome(self, testNames, output=sys.stdout, saveOutput=False):
        self.manager.runSomeTests(testNames, output=output, saveOutput=saveOutput)

    def runAll(self, output=sys.stdout, saveOutput=False):
        self.manager.runAllTests(output=output, saveOutput=saveOutput)
