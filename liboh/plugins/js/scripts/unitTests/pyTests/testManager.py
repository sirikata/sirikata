#!/usr/bin/python

from __future__ import print_function
import sys
import os;
import shutil;
import time;

import os.path

_this_script_dir = os.path.dirname(__file__)

DEFAULT_OUTPUT_FILENAME = 'unitTestResults.txt';
DEFAULT_BIN_PATH = os.path.join(_this_script_dir, '../../../../../../build/cmake/')
DEFAULT_CPPOH_BIN_NAME = 'cppoh_d'
DEFAULT_SPACE_BIN_NAME = 'space_d'

DEFAULT_DIRTY_FOLDER = 'pyTests/DIRTY_UNIT_TEST_DIR';



class TestManager:
    def __init__(self):
        # Maintain an array and a dict -- the dict for easy lookup,
        # the array to allow us to keep things ordered so tests can
        # run in the sequence we want
        self.mTests = [];
        self._testsByName = {}

    def addTest(self,testToAdd):
        self.mTests.append(testToAdd);
        self._testsByName[testToAdd.testName] = testToAdd

    def addTestArray(self,testArrayToAdd):
        for s in testArrayToAdd:
            self.addTest(s);

    def runSomeTests(self, testNames, output=sys.stdout, binPath=DEFAULT_BIN_PATH, cppohBinName=DEFAULT_CPPOH_BIN_NAME, spaceBinName=DEFAULT_SPACE_BIN_NAME, saveOutput=False):
        numTests = len(testNames);
        count = 1;

        #if the dirty folder already exists then delete it first
        if(os.path.exists(DEFAULT_DIRTY_FOLDER)):
            print('WARNING: deleting previous output folder at ' + DEFAULT_DIRTY_FOLDER, file=output);
            shutil.rmtree(DEFAULT_DIRTY_FOLDER);

        succeeded = 0
        failed = 0
        for s in testNames:
            print("*" * 40, file=output)
            print('BEGIN TEST', s, '(' + str(count) + ' of ' + str(numTests) + ')', file=output)

            folderName = os.path.join(DEFAULT_DIRTY_FOLDER, s);
            if (os.path.exists(folderName)):
                print('WARNING: deleting previous output file at ' + folderName, file=output);
                shutil.rmtree(folderName);

            os.makedirs(folderName);
            test = self._testsByName[s];
            success = test.runTest(folderName,binPath=binPath, cppohBinName=cppohBinName, spaceBinName=spaceBinName, output=output)
            if success:
                succeeded += 1
            else:
                failed += 1

            print('END TEST', '(' + str(count) + ' of ' + str(numTests) + ')', file=output)
            print("*" * 40, file=output)
            print(file=output)

            count += 1;

        print(file=output)
        print("*" * 40, file=output)
        print("SUMMARY:", succeeded, "succeeded,", failed, "failed", file=output)
        print("*" * 40, file=output)

        if (not saveOutput):
            shutil.rmtree(DEFAULT_DIRTY_FOLDER);


    def runAllTests(self, output=sys.stdout, binPath=DEFAULT_BIN_PATH, cppohBinName=DEFAULT_CPPOH_BIN_NAME, spaceBinName=DEFAULT_SPACE_BIN_NAME, saveOutput=False):
        testNames = [t.testName for t in self.mTests];
        self.runSomeTests(testNames, output=output, binPath=binPath, cppohBinName=cppohBinName, spaceBinName=spaceBinName, saveOutput=saveOutput)
