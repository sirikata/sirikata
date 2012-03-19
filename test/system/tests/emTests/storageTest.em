/**
 Duration 10s
 Scene.db:
    *Ent 1: Anything.

 Tests:
   -system.storageBeginTransaction
   -system.storageCommit
   -system.storageErase
   -system.storageRead
   -system.storageWrite
   -system.storageCount
   -serialization
   -timeout
   -killEntity
 */


system.require('helperLibs/util.em');
mTest = new UnitTest('storageTest.em');
var toSerialize = {'x':1,'y':2};

//incremented once for each test that we run.  in wrapUp, we compare
//to NUM_TESTS to ensure that we actually got to all the tests.
var numCallbacks = 0;
var NUM_TESTS    = 8;


//used by testWrite and write field and erase written
var fieldKey = 'mFieldKey';

//if we don't finish all these tests in 10 seconds, then we'll consider this a failure
system.timeout(10, wrapUp);

//read unknown field
system.storageRead('noExist', function (success, val)
                   {
                       ++numCallbacks;
                       if (success)
                           mTest.fail('Got successful read for imaginary data');
                       if (typeof(val) !== 'undefined')
                           mTest.fail('Got non-undefined read for imaginary data');
                       testWrite();
                   });


//write toSerialize
function testWrite()
{
    var serialized = system.serialize(toSerialize);
    system.storageWrite(fieldKey,serialized,
                        function(success)
                        {
                            ++numCallbacks;
                            if (typeof(success) !== 'boolean')
                                mTest.fail('Did not receive boolean argument success in testWrite');
                            if (!success)
                                mTest.fail('Failed to write in testWrite');

                            testReadWrittenField();
                        }
                       );
}

//checks if the field written in testWrite actually was written correctly.
function testReadWrittenField()
{
    system.storageRead(fieldKey,
                       function(success,val)
                       {
                           ++numCallbacks;
                           if (typeof(success) !== 'boolean')
                               mTest.fail('Did not receive boolean argument success in testReadWrittenField');
                           if (!success)
                               mTest.fail('Failed to read in testReadWrittenField');
                           else
                           {
                               if (typeof(val) !== 'object')
                               {
                                   mTest.fail('Error in testReadWrittenField.  Should have received an object back from storage system.');
                               }
                               else if (!(fieldKey in val))
                               {
                                   mTest.fail('Error in testReadWrittenField.  Returned value should have a field named by fieldKey');
                               }
                               else
                               {
                                   compareTwoBasicObjects(system.deserialize(val[fieldKey]),toSerialize,'testReadWrittenField');                                           
                               }

                           }
                           testEraseUnknown();
                       }
                      );
}

//tries to erase a field that doesn't exist
function testEraseUnknown()
{
    system.storageBeginTransaction();
    system.storageErase('noExist');
    system.storageCommit( function (success)
                   {
                       ++numCallbacks;
                       if (!success)
                           mTest.fail('Erasing non-existant data failed: ' + success);

                       testEraseWrittenField();
                   });
}

//tries to erase field that was written with key fieldKey in testWrite 
function testEraseWrittenField()
{
    system.storageBeginTransaction();
    system.storageErase(fieldKey);

    system.storageCommit( function (success)
                   {
                       ++numCallbacks;
                       if (!success)
                           mTest.fail('Error in testEraseWrittenField: did not erase data that was written.');

                       testReadErased();
                   });
}

//tries to read the field that was erased in testEraseWrittenField
function testReadErased()
{
    system.storageRead(fieldKey,
                       function(success,val)
                       {
                           ++numCallbacks;
                           if (typeof(success) !== 'boolean')
                               mTest.fail('Did not receive boolean argument success in testReadErased');
                           if (success)
                               mTest.fail('Error in testReadErased: could read a field that had been erased.');

                           if (typeof(val) !== 'undefined')
                               mTest.fail('Error in testReadErased: should have received an undefined value');

                           testBasicTransaction();
                       }
                      );
}


//Inside of a transaction, reads an unknown field and writes a field.
//If transactions, are working correctly
function testBasicTransaction()
{
    system.storageBeginTransaction();
    system.storageRead('noExist');
    system.storageWrite(fieldKey,system.serialize(toSerialize));
    system.storageCommit(function(success,readVal)
                         {
                             ++numCallbacks;
                             
                             if (success)
                                 mTest.fail('Error in testBasicTransaction.  Read should have failed.');

                             if (typeof(readVal) !== 'undefined')
                                 mTest.fail('Error in testBasicTransaction.  Read should have failed, which implies we should not have gotten an object back');


                             //try reading the key that shouldn't have been written
                             system.storageRead(fieldKey,
                                                function (success,val)
                                                {
                                                    if (success)
                                                        mTest.fail('Error in testBasicTransaction.  Transaction should have failed, and value should not have been written.');

                                                    if (typeof(val) !== 'undefined')
                                                        mTest.fail('Error in testBasicTransaction.  Transaction should have failed, and should not get data back in val.');


                                                    testTwoOperationTransaction();
                                                }
                                               );
                         }
    );
}



function testTwoOperationTransaction()
{
    //do two writes first
    system.storageBeginTransaction();
    var fieldKey2 = fieldKey + '2';
    system.storageWrite(fieldKey,system.serialize(toSerialize));
    system.storageWrite(fieldKey2,system.serialize(toSerialize));
    
    system.storageCommit(function(success)
                        {
                           ++numCallbacks;
                            
                            if (!success)
                                mTest.fail('Error in testTwoOperationTransactions when writing initial values.  Should have been written.');



                            system.storageBeginTransaction();
                            system.storageRead(fieldKey);
                            system.storageRead(fieldKey2);
                            system.storageCommit(
                                function (success, val)
                                {
                                    if (!success)
                                        mTest.fail('Error in testTwoOperationTransactions when reading written values.  Got no success');



                                    if (typeof(val) !== 'object')
                                        mTest.fail('Error in testTwoOperationTransactions: did not get an object back.');
                                    else
                                    {
                                        if ((!(fieldKey in val)) && (!(fieldKey2 in val)))
                                            mTest.fail('Error in testTwoOperaionTransactions: returned object does not have correct keys');
                                        else
                                        {
                                            compareTwoBasicObjects(system.deserialize(val[fieldKey]), toSerialize,'testTwoOperationTransaction');
                                            compareTwoBasicObjects(system.deserialize(val[fieldKey2]), toSerialize,'testTwoOerationTransaction 2');
                                        }
                                    }
                                    testCount();
                                }
                            );                            
                                
                        });
    
}


//write toSerialize
function testCount()
{
    // Write some new data, within a range, we can
    // count. Values don't matter. Clear out all other data first to
    // make sure we won't conflict with other tests.
    system.storageRangeErase(
        'a', 'z',
        function(success) {
            if (!success)
                mTest.fail('Failed to clear data testCount');
            
            system.storageBeginTransaction();
            system.storageWrite('ddd',system.serialize(toSerialize));
            system.storageWrite('xxx',system.serialize(toSerialize));
            system.storageCommit(
                function(success) {
                    if (!success)
                        mTest.fail('Failed to write data for testCount');

                    // Now perform the count
                    system.storageCount(
                        'a', 'z',
                        function(success, count) {
                            if (!success)
                                mTest.fail('Failed to execute count request');
                            if (count !== 2)
                                mTest.fail('Incorrect number of objects returned: ' + count + ' instead of 2');

                            system.storageRangeErase(
                                'a', 'z',
                                function(success) {
                                    if (!success)
                                        mTest.fail('Failed to clear data after testCount');
                                    wrapUp();
                                }
                            );
                        }
                    );
                }
            );
        }
    );
}


//prints success if haven't gotten any failures. Checks that we hit all the callbacks, kills entity.
function wrapUp()
{
    if (numCallbacks != NUM_TESTS)
        mTest.fail('Error in wrapup.  Did not execute to completion.');

    if (! mTest.hasFailed)
        mTest.success('Successfully completed all storageTests.');

    system.killEntity();
}



//returns true if the objects match up (have same fields and same data in fields).
function compareTwoBasicObjects(obj1,obj2,where)
{
    return (compareFieldsInOneToTwo(obj1, obj2, where) &&
            compareFieldsInOneToTwo(obj2, obj1, where));
}

function compareFieldsInOneToTwo(obj1, obj2, where)
{
    if ((typeof(obj1) !== 'object') || (typeof(obj2) !== 'object'))
    {
        mTest.fail('in ' + where + ' .  Did not get two objects to compare');
        return false;
    }
    
    for (var s in obj1)
    {
        if (! (s in obj2))
        {
            mTest.fail('error in ' + where + ' field mismatch.');
            system.__debugPrint('\n\nField name ' + s.toString() + ' is missing.');
            system.__debugPrint('\n\n');
            system.prettyprint(obj2);
            system.__debugPrint('\n\n');
            system.prettyprint(obj1);
            system.__debugPrint('\n\n');
            return false;
        }
        else
        {
            if (obj1[s] !== obj2[s])
            {
                mTest.fail('error in ' + where + ' data mismatch.');
                return false;
            }
        }
    }
    
    return true;
}
