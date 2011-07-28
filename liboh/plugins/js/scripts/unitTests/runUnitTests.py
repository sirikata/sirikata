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

from csvConstructorInfo import *

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
                                howLongToRunInSeconds=20
                                );
    
    testArray.append(basicTest);

    
    #proximityAdded test: see documentation in unitTests/emTests/proximityAdded.em.
    #Tests: onProxAdded, setQueryAngle, setVelocity, createPresence,
    #       getProxSet, timeout, onPresenceConnected
    otherEntInfo = CSVConstructorInfo(
        pos_x=numWrap(10),
        pos_y=numWrap(0),
        pos_z=numWrap(0),
        scale=numWrap(1),
        solid_angle=numWrap(100)
        );
    
    csvProxAddedEntInfo = CSVConstructorInfo(
        script_type=stringWrap("js"),
        script_contents=stringWrap("system.import('unitTests/emTests/proximityAdded.em');"),
        pos_x=numWrap(0),
        pos_y=numWrap(0),
        pos_z=numWrap(0),
        scale=numWrap(1),
        solid_angle=numWrap(100)
        );
    
    proximityAddedTest = csvTest.CSVTest("proximityAdded",
                                         
                                         touches=['onProxAdded',
                                                  'setQueryAngle',
                                                  'setVelocity',
                                                  'createPresence',
                                                  'getProxSet',
                                                  'timeout',
                                                  'onPresenceConnected'],
                                         
                                         entityConstructorInfo=[otherEntInfo,
                                                                csvProxAddedEntInfo],
                                             
                                         errorConditions=[basicErrors.SegFaultError,
                                                          basicErrors.BusError,
                                                          basicErrors.AssertError,
                                                          basicErrors.UnitTestNoSuccessError,
                                                          basicErrors.UnitTestFailError],
                                             
                                         howLongToRunInSeconds=50
                                         );
    
    testArray.append(proximityAddedTest);


    
                    

    

    ##create manager and populate it with test array.
    manager = testManager.TestManager();
    manager.addTestArray(testArray);
    manager.runAllTests();
    

if __name__ == "__main__":
    runAll();


    
