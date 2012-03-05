#!/usr/bin/python

from __future__ import print_function
import sys

import pyTests.singleTest as singleTest
import dbGen.csvGenerator as csvGenerator
import os
import subprocess
from procset import ProcSet

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

        outputCatcher = open(runOutputFilename,'w')

        popenCall =['./'+cppohBinName];
        for s in self.additionalCMDLineArgs:
            popenCall.append(s);

        prevDir = os.getcwd();
        os.chdir(cppohPath);

        procs = ProcSet()
        procs.process(popenCall, stdout=outputCatcher, stderr=subprocess.STDOUT, default=True)
        procs.run(waitUntil=self.duration, killAt=self.duration+10, output=output)

        # Print a notification if we had to kill this process
        if procs.hupped():
            print(file=outputCatcher)
            print('UNIT_TEST_TIMEOUT', file=outputCatcher)

        outputCatcher.close();
        os.chdir(prevDir);

        return self.analyzeOutput(runOutputFilename, procs.returncode(), output=output);


    def analyzeOutput(self, filenameToAnalyze, returnCode, output):
        success = super(CSVTest, self).analyzeOutput(filenameToAnalyze, returnCode, output=output)
        if not success:
            print("Execution Log:", file=output)
            fp = open(filenameToAnalyze, 'r')
            for line in fp.readlines():
                print("    ", line, end='', file=output)
            fp.close()
            print(file=output)
        return success
