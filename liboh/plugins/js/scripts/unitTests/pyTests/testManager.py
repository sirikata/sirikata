#!/usr/bin/python

import os;
import shutil;
import time;

DEFAULT_OUTPUT_FILENAME = 'unitTestResults.txt';
DEFAULT_CPPOH_PATH = '../../../../../build/cmake/';
DEFAULT_CPPOH_BIN_NAME = 'cppoh_d';

DEFAULT_DIRTY_FOLDER = 'pyTests/DIRTY_UNIT_TEST_DIR';



class TestManager:
    def __init__(self):
        self.mTests = [];

    def addTest(self,testToAdd):
        self.mTests.append(testToAdd);

    def addTestArray(self,testArrayToAdd):
        for s in testArrayToAdd:
            self.addTest(s);

    def runAllTests(self, outputFilename=DEFAULT_OUTPUT_FILENAME,cppohPath=DEFAULT_CPPOH_PATH, cppohBinName=DEFAULT_CPPOH_BIN_NAME):
        numTests = len(self.mTests);
        count = 1;

        #if the output file already existed, then delete it.
        if(os.path.exists(outputFilename)):
            os.remove(outputFilename);
        
        for s in self.mTests:
            print('\nRunning test ' + str(count) + ' of ' + str(numTests) + '\n');
            ++count;

            #if the dirty folder already exists then delete it first
            if(os.path.exists(DEFAULT_DIRTY_FOLDER)):
                shutil.rmtree(DEFAULT_DIRTY_FOLDER);

            
            os.mkdir(DEFAULT_DIRTY_FOLDER);
            s.runTest(outputFilename,DEFAULT_DIRTY_FOLDER,cppohPath,cppohBinName);
            shutil.rmtree(DEFAULT_DIRTY_FOLDER);

            
            
        
    
