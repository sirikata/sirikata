#!/usr/bin/python

import sys;
sys.path.append('pyTests/');
sys.path.append('dbGen/');
sys.path.append('errorConditions/');

import testManager
import csvTest
import csvConstructorInfo
import basicGenerators
import basicErrors

def runAll():

    testArray = [];

    '''
    This is where you add in you tests.
    '''
    
    #create basic test: single entity runs for 10 seconds and shuts
    #down.
    basicTest = csvTest.CSVTest("basicTest",
                                [basicGenerators.KillAfterTenSecondsEntity],
                                [basicErrors.SegFaultError,
                                 basicErrors.BusError,
                                 basicErrors.AssertError,
                                 basicErrors.UnitTestNoSuccessError],
                                );
    
    testArray.append(basicTest);

                    

    

    ##create manager and populate it with test array.
    manager = testManager.TestManager();
    manager.addTestArray(testArray);
    manager.runAllTests();
    

if __name__ == "__main__":
    runAll();


    
