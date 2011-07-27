#!/usr/bin/python


import sys
sys.path.append("../dbGen/")

import singleTest
import csvGenerator
import subprocess
import os
import datetime
import time


CSV_DB_FILENAME = 'unit_test_csv_db.db';
RUN_OUTPUT_FILENAME = 'runOutput';


class CSVTest(singleTest.SingleTest):

    '''
    
    @param {Array}  additionalCMDLineArgs Specify any additional
    arguments you want to provide to cpp_oh.  Note do not specify
    object-factory or object-factory-opts (they're being used by this class).
    '''
    def __init__(self,testName,entityConstructorInfo=[],errorConditions=[], additionalCMDLineArgs=[]):
        self.testName = testName;
        self.errorConditions = errorConditions;
        self.csvGen = csvGenerator.CSVGenerator(entityConstructorInfo);
        self.additionalCMDLineArgs = additionalCMDLineArgs;

        
    def addEntityConstructorInfo(self,eci):
        self.csvGen.addEntity(eci);

    def addEntityArrayConstructorInfo(self,arrayeci):
        self.csvGen.addArrayOfEntities();


    '''
    Space should already be running.
    
    @param {String} outputFileName Appends the results of all the
    error tests to the file with this name.

    @param {String} dirtyFolderName The testManager guarantees that a
    folder exists with this name that csvTest can write arbitrary
    files to without interfering with anything else that's running.
    The testManager will delete this folder after runTest returns.

    @param {String} cppohPath should be the name of the
    directory that cppoh is in.

    @param {String} cppohBinName The actual name of the
    binary/executale to run.

    '''
    def runTest(self,outputFilename,dirtyFolderName,cppohPath, cppohBinName, howLongToRunInSeconds):
        dbFilename = os.path.join(dirtyFolderName,CSV_DB_FILENAME);
        runOutputFilename = os.path.join(dirtyFolderName,RUN_OUTPUT_FILENAME);
        
        #create the db file to read from.
        self.csvGen.writeDB(dbFilename);
        self.additionalCMDLineArgs.append('--object-factory=csv');
        self.additionalCMDLineArgs.append('--object-factory-opts=--db='+ os.path.abspath(dbFilename));


        
        
        print('\n\n******************************\n');
        print('\nRunning test ' + self.testName + '\n');
        outputCatcher = open(runOutputFilename,'w');

        popenCall =['./'+cppohBinName];
        for s in self.additionalCMDLineArgs:
            popenCall.append(s);
        
        prevDir = os.getcwd();
        os.chdir(cppohPath);
        start = datetime.datetime.now();
        proc = subprocess.Popen(popenCall,  stdout=outputCatcher, stderr=outputCatcher);

        
        while proc.poll() is None:
            time.sleep(1)
            now = datetime.datetime.now();
            if (now - start).seconds > howLongToRunInSeconds:
                proc.kill();
        

        outputCatcher.flush();
        outputCatcher.close();
        os.chdir(prevDir);
        
        print('\nAnalyzing test ' + self.testName + '\n');
        self.analyzeOutput(runOutputFilename, outputFilename);
        print('\nDone with test ' + self.testName + '\n');
        
        
        
        
        
        
    
        
        
    

