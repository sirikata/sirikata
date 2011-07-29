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

    def runAllTests(self, outputFilename=DEFAULT_OUTPUT_FILENAME,cppohPath=DEFAULT_CPPOH_PATH, cppohBinName=DEFAULT_CPPOH_BIN_NAME,saveOutput=False):
        numTests = len(self.mTests);
        count = 1;

        #if the output file already existed, then delete it.
        if(os.path.exists(outputFilename)):
            os.remove(outputFilename);

        #if the dirty folder already exists then delete it first
        if(os.path.exists(DEFAULT_DIRTY_FOLDER)):
            print('WARNING: deleting previous output folder at ' + DEFAULT_DIRTY_FOLDER);
            shutil.rmtree(DEFAULT_DIRTY_FOLDER);

        for s in self.mTests:
            print('\nRunning test ' + str(count) + ' of ' + str(numTests) + '\n');
            count += 1;


            folderName = os.path.join(DEFAULT_DIRTY_FOLDER, s.testName);
            if (os.path.exists(folderName)):
                print('WARNING: deleting previous output file at ' + folderName);
                shutil.rmtree(folderName);
            
            os.makedirs(folderName);
            s.runTest(outputFilename,folderName,cppohPath,cppohBinName);

            
        if (not saveOutput):
            shutil.rmtree(DEFAULT_DIRTY_FOLDER);

            
            
        
    
