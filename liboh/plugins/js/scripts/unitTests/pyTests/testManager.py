#!/usr/bin/python

import os;
import shutil;
import time;

import os.path

_this_script_dir = os.path.dirname(__file__)

DEFAULT_OUTPUT_FILENAME = 'unitTestResults.txt';
DEFAULT_CPPOH_PATH = os.path.join(_this_script_dir, '../../../../../../build/cmake/')
DEFAULT_CPPOH_BIN_NAME = 'cppoh_d';

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

    def runSomeTests(self, testNames, outputFilename=DEFAULT_OUTPUT_FILENAME,cppohPath=DEFAULT_CPPOH_PATH, cppohBinName=DEFAULT_CPPOH_BIN_NAME,saveOutput=False):
        numTests = len(testNames);
        count = 1;

        #if the output file already existed, then delete it.
        if(os.path.exists(outputFilename)):
            os.remove(outputFilename);

        #if the dirty folder already exists then delete it first
        if(os.path.exists(DEFAULT_DIRTY_FOLDER)):
            print('WARNING: deleting previous output folder at ' + DEFAULT_DIRTY_FOLDER);
            shutil.rmtree(DEFAULT_DIRTY_FOLDER);

        for s in testNames:
            print('\nRunning test ' + str(count) + ' of ' + str(numTests) + '\n');
            count += 1;

            folderName = os.path.join(DEFAULT_DIRTY_FOLDER, s);
            if (os.path.exists(folderName)):
                print('WARNING: deleting previous output file at ' + folderName);
                shutil.rmtree(folderName);

            os.makedirs(folderName);
            test = self._testsByName[s];
            test.runTest(outputFilename,folderName,cppohPath,cppohBinName);

        if (not saveOutput):
            shutil.rmtree(DEFAULT_DIRTY_FOLDER);


    def runAllTests(self, outputFilename=DEFAULT_OUTPUT_FILENAME,cppohPath=DEFAULT_CPPOH_PATH, cppohBinName=DEFAULT_CPPOH_BIN_NAME,saveOutput=False):
        testNames = [t.testName for t in self.mTests];
        self.runSomeTests(testNames, outputFilename=outputFilename, cppohPath=cppohPath, cppohBinName=cppohBinName, saveOutput=saveOutput)
