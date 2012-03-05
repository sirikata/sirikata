#!/usr/bin/python

from __future__ import print_function
import sys

import pyTests.singleTest as singleTest
import dbGen.csvGenerator as csvGenerator
import os
import subprocess
from procset import ProcSet
import random

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

    @param {String} binPath should be the name of the
    directory that cppoh is in.

    @param {String} cppohBinName The actual name of the
    binary/executale to run.

    '''
    def runTest(self, dirtyFolderName, binPath, cppohBinName, spaceBinName, output=sys.stdout):
        dbFilename = os.path.join(dirtyFolderName, 'unit_test_csv_db.db')
        cppoh_output_filename = os.path.join(dirtyFolderName, 'cppoh.log')
        space_output_filename = os.path.join(dirtyFolderName, 'space.log')

        # Random port to avoid conflicts
        port = 2000 + random.randint(0, 1000) % 1000
        # Space
        space_cmd = ['./'+spaceBinName]
        space_cmd.append('--servermap-options=--port=' + str(port))
        # OH - create the db file to read from.
        self.csvGen.writeDB(dbFilename);
        cppoh_cmd =['./'+cppohBinName];
        cppoh_cmd.append('--servermap-options=--port=' + str(port))
        for s in self.additionalCMDLineArgs:
            cppoh_cmd.append(s);
        cppoh_cmd.append('--object-factory=csv');
        cppoh_cmd.append('--object-factory-opts=--db='+ os.path.abspath(dbFilename));

        prevDir = os.getcwd();
        os.chdir(binPath);

        procs = ProcSet()
        space_output = open(space_output_filename, 'w')
        procs.process(space_cmd, stdout=space_output, stderr=subprocess.STDOUT, wait=False)
        cppoh_output = open(cppoh_output_filename,'w')
        procs.process(cppoh_cmd, stdout=cppoh_output, stderr=subprocess.STDOUT, at=3, default=True)

        procs.run(waitUntil=self.duration, killAt=self.duration+10, output=output)

        # Print a notification if we had to kill this process
        if procs.hupped():
            print(file=cppoh_output)
            print('UNIT_TEST_TIMEOUT', file=cppoh_output)

        space_output.close();
        cppoh_output.close();
        os.chdir(prevDir);

        return self.analyzeOutput(
            analyze=cppoh_output_filename,
            report={ 'Object Host': cppoh_output_filename, 'Space Server': space_output_filename},
            returnCode=procs.returncode(), output=output
            )


    def analyzeOutput(self, analyze, report, returnCode, output):
        success = super(CSVTest, self).analyzeOutput(analyze, returnCode, output=output)
        if not success:
            print("Execution Log:", file=output)
            for report_name, report_file in report.iteritems():
                print("  ", report_name, file=output)
                fp = open(report_file, 'r')
                for line in fp.readlines():
                    print("    ", line, end='', file=output)
                fp.close()
                print(file=output)
        return success
