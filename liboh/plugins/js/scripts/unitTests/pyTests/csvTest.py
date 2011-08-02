#!/usr/bin/python


import sys

import pyTests.singleTest as singleTest
import dbGen.csvGenerator as csvGenerator
import subprocess
import os
import datetime
import time
import signal

CSV_DB_FILENAME = 'unit_test_csv_db.db';
RUN_OUTPUT_FILENAME = 'runOutput';


class CSVTest(singleTest.SingleTest):

    '''
    @see singleTest.SingleTest
    '''
    def __init__(self,testName,entityConstructorInfo=None,errorConditions=None, additionalCMDLineArgs=None,howLongToRunInSeconds=singleTest.DEFAULT_TEST_TIME_IN_SECONDS, touches=None):
        
        singleTest.SingleTest.__init__(self,testName,errorConditions,additionalCMDLineArgs,howLongToRunInSeconds,touches);

        self.csvGen = csvGenerator.CSVGenerator(entityConstructorInfo);
        
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
    def runTest(self,outputFilename,dirtyFolderName,cppohPath, cppohBinName):
        dbFilename = os.path.join(dirtyFolderName,CSV_DB_FILENAME);
        runOutputFilename = os.path.join(dirtyFolderName,RUN_OUTPUT_FILENAME);
        
        #create the db file to read from.
        self.csvGen.writeDB(dbFilename);
        self.additionalCMDLineArgs.append('--object-factory=csv');
        self.additionalCMDLineArgs.append('--object-factory-opts=--db='+ os.path.abspath(dbFilename));


        print('Running test ' + self.testName + '\n');
        outputCatcher = open(runOutputFilename,'w');

        popenCall =['./'+cppohBinName];
        for s in self.additionalCMDLineArgs:
            popenCall.append(s);
        
        prevDir = os.getcwd();
        os.chdir(cppohPath);
        start = datetime.datetime.now();

        
        proc = subprocess.Popen(popenCall,stdout=outputCatcher, stderr=subprocess.STDOUT);
        signalSent = False;
        while proc.poll() is None:
            time.sleep(1)
            now = datetime.datetime.now();
            if (((now - start).seconds > self.howLongToRunInSeconds) and (not signalSent)):
                sys.stdout.flush();
                signalSent = True;
                print('Sending sighup to object host');
                sys.stderr.flush();
                proc.send_signal(signal.SIGHUP);

                
        outputCatcher.close();
        os.chdir(prevDir);
        
        print('Analyzing test ' + self.testName + '\n');
        self.analyzeOutput(runOutputFilename, outputFilename);
        print('\nDone with test ' + self.testName + '\n');
        
        
        
        
        
        
    
        
        
    
