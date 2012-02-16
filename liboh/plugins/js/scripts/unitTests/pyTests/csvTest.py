#!/usr/bin/python

from __future__ import print_function
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
    def __init__(self, name, entityConstructorInfo=None, **kwargs):
        singleTest.SingleTest.__init__(self,name=name,**kwargs)
        self.csvGen = csvGenerator.CSVGenerator(entityConstructorInfo);
        
    def addEntityConstructorInfo(self,eci):
        self.csvGen.addEntity(eci);

    def addEntityArrayConstructorInfo(self,arrayeci):
        self.csvGen.addArrayOfEntities();


    '''
    Space should already be running.    

    @param {String} dirtyFolderName The testManager guarantees that a
    folder exists with this name that csvTest can write arbitrary
    files to without interfering with anything else that's running.
    The testManager will delete this folder after runTest returns.

    @param {String} cppohPath should be the name of the
    directory that cppoh is in.

    @param {String} cppohBinName The actual name of the
    binary/executale to run.

    '''
    def runTest(self, dirtyFolderName, cppohPath, cppohBinName, output=sys.stdout):
        dbFilename = os.path.join(dirtyFolderName,CSV_DB_FILENAME);
        runOutputFilename = os.path.join(dirtyFolderName,RUN_OUTPUT_FILENAME);
        
        #create the db file to read from.
        self.csvGen.writeDB(dbFilename);
        self.additionalCMDLineArgs.append('--object-factory=csv');
        self.additionalCMDLineArgs.append('--object-factory-opts=--db='+ os.path.abspath(dbFilename));

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
            if (((now - start).seconds > self.duration) and (not signalSent)):
                sys.stdout.flush();
                signalSent = True;
                print('Sending SIGHUP to object host', file=output);
                sys.stderr.flush();
                proc.send_signal(signal.SIGHUP)
                # In case a SIGHUP isn't enough, wait a bit longer and try to kill
                while proc.poll() is None:
                    time.sleep(1)
                    now = datetime.datetime.now();
                    if (now - start).seconds > self.duration + 10:
                        # Send signal twice, in case the first one is
                        # caught. The second should (in theory) always
                        # kill the process as it should have disabled
                        # the handler
                        print('Sending SIGKILL to object host', file=output);
                        proc.send_signal(signal.SIGKILL)
                        proc.send_signal(signal.SIGKILL)
                        proc.wait()

                # Print a notification that we had to kill this process
                print(file=outputCatcher)
                print('UNIT_TEST_TIMEOUT', file=outputCatcher)

                
        outputCatcher.close();
        os.chdir(prevDir);

        return self.analyzeOutput(runOutputFilename, proc.returncode, output=output);
        
        
        
        
        
    
        
        
    
