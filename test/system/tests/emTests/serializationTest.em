/**
 serializationTest -- Tests serialization of arrays, objects, and
 functions.  Note, still missing tests for serializing presences,
 visibles, etc.
 
  Scene.db:
  * Ent 1 : Anything

  Duration 10s
 
 */

system.require('helperLibs/util.em');
mTest = new UnitTest('serializationTest');

system.onPresenceConnected(runTests);

hasFailed = false;
function failed(failText)
{
    mTest.fail(failText);
    hasFailed = true;
}

function runTests()
{
    emptyObject();
    emptyArray();
    emptyFunction();
    basicFunction();
    populatedObject();
    nestedObject();
    loopedObject();
    populatedArray();
    funcWithFields();
    prototypeObject();
    prototypeChain();

    undefinedObject();
    nullObject();
    
    if (!hasFailed)
        mTest.success('Got all the way through serializations.');

    system.killEntity();
}

//testSerialize empty object
function emptyObject()
{
    var empty = {};
    var serialized   = system.serialize(empty);
    var deserialized = system.deserialize(serialized);

    if (typeof(deserialized) != 'object' )
        failed('error serializing empty object.  did not retain type information.');
    else
    {
        for (var s in deserialized)
            failed ('error serializing empty object.  it should not have any fields');
    }
}

//testSerialize array
function emptyArray()
{
    var empty = [];
    var serialized   = system.serialize(empty);
    var deserialized = system.deserialize(serialized);

    if (typeof(deserialized) != 'object' )
        failed('error serializing empty array.  did not retain type information.');

    if (!('length' in deserialized))
        failed('error serializing empty array.  does not have length information.');
    else
        if (deserialized.length !== 0)
            failed('error serializing empty array.  incorrect length information.');

    try
    {
        deserialized.push(1);
        deserialized.push(3);
        deserialized.push(5);

        if (deserialized.length !== 3)
            failed('error serializing empty array.  pushing items did not yield correct length.');

        if ((deserialized[0] != 1) || (deserialized[1] != 3) || (deserialized[2] != 5))
            failed('error serializing empty array.  pushed items do not still exist on array.');

        deserialized.reverse();
        if ((deserialized[0] != 5) || (deserialized[1] != 3) || (deserialized[2] != 1))
            failed('error serializing empty array.  reverse call is incorrect.');
    }
    catch(excep)
    {
        failed('error performing array operations on deserialized empty array.  here is exception: ' + excep.toString());        
    }
}


function emptyFunction()
{
    var empty = function(){};
    var prevAsString = empty.toString();
    var serialized   = system.serialize(empty);
    var deserialized = system.deserialize(serialized);

    if (typeof(deserialized) != 'function' )
        failed('error serializing empty function.  did not retain type information.');

    
    try
    {
        deserialized();
    }
    catch(excep)
    {
        failed('error serializing empty function.  exception when trying to execute function.');
    };
}


function basicFunction()
{
    var toSerialize  = function(a,b){ return a+b;};
    var serialized   = system.serialize(toSerialize);
    var deserialized = system.deserialize(serialized);

    if (typeof(deserialized) != 'function')
        failed('error serializing basic function.  did not retain type information.');

    for (var s =0; s < 10; ++s)
    {
        var lhs = Math.random();
        var rhs = Math.random();
        if (toSerialize(lhs,rhs) !== deserialized(lhs,rhs))
            failed('error serializing basic function.  functions returned different values for same input.');
    }

}

function populatedObject()
{
    var toSerialize = { '0':'a',
                        '1':'b',
                        '2': 333,
                        '3': false,
                        '4': true,
                        '5': 81.2,
                        '6': -1.3,
                        '7': function (a,b){ return a+b;}};

    var serialized = system.serialize(toSerialize);
    var deserialized = system.deserialize(serialized);

    if (typeof(deserialized) != 'object')
        failed('error serializing populated object.  did not retain type information.');

    //note that this doesn't actually check the function object on the end
    //(it won't be identical to the real function).
    var sameFields= compareObjs(toSerialize,deserialized,'populated object test');

    if (sameFields)
    {
        //test that the functions return the same things for various input.
        for (var s =0; s < 10; ++s)
        {
            var lhs = Math.random();
            var rhs = Math.random();
            
            var beforeFunc = toSerialize['7'];
            var afterFunc  = deserialized['7'];
        
            if (beforeFunc(lhs,rhs) !== afterFunc(lhs,rhs))
                failed('error serializing populated object.  function field did not return same result pre and post serialization.');
        }
    }
    
}


function nestedObject()
{
    var toSerialize = {'0': '0',
                       '1': { '0': 'data',
                              '1': {}
                            },
                       '2': 3,
                       '3': { '0': 20,
                              '1': function (a,b){return a+b;},
                              '2': { '0': true}
                              }
                      };

    var serialized = system.serialize(toSerialize);
    var deserialized = system.deserialize(serialized);

    if (typeof(deserialized) != 'object')
        failed('error serializing nested object.  did not retain type information.');

    var sameFields = compareObjs(toSerialize,deserialized,'nestedObject');
    if (sameFields)
    {
        var beforeFuncToTest= toSerialize['3']['1'];
        var afterFuncToTest = deserialized['3']['1'];

        for (var s =  0; s < 10; ++s)
        {
            var lhs = Math.random();
            var rhs = Math.random();
            if (beforeFuncToTest(lhs,rhs) !== afterFuncToTest(lhs,rhs))
                failed('error serializing nested object.  functions produced distinct results');
        }
    }
}


function loopedObject()
{
    var toSerialize = {'0': '0',
                       '1': { '0': 'data',
                              '1': {}
                            },
                       '2': 3,
                       '3': { '0': 20,
                              '1': function (a,b){return a+b;},
                              '2': { '0': true}
                              }
                      };

    toSerialize['a'] = toSerialize;
    toSerialize['1']['b'] = toSerialize;
    
    var serialized = system.serialize(toSerialize);
    var deserialized = system.deserialize(serialized);

    if (typeof(deserialized) != 'object')
        failed('error serializing nested object.  did not retain type information.');

    var sameFields = compareObjs(toSerialize,deserialized,'nestedObject');
    if (sameFields)
    {
        var beforeFuncToTest= toSerialize['3']['1'];
        var afterFuncToTest = deserialized['3']['1'];

        for (var s =  0; s < 10; ++s)
        {
            var lhs = Math.random();
            var rhs = Math.random();
            if (beforeFuncToTest(lhs,rhs) !== afterFuncToTest(lhs,rhs))
                failed('error serializing nested object.  functions produced distinct results');
        }
    }
}


function populatedArray()
{
    var toSerialize = [1,2,3,{},'abcd',false,true];
    var serialized = system.serialize(toSerialize);
    var deserialized = system.deserialize(serialized);
    

    if (typeof(deserialized) != 'object')
        failed('error serializing populated array.  did not retain type information.');

    var sameFields = compareObjs(toSerialize,deserialized,'populated array');
}


function funcWithFields()
{
    var toSerialize = function(a,b){ return a + b;};
    toSerialize.f1  = 'a';
    toSerialize.f2  = 3;
    toSerialize.f3  = toSerialize;

    var serialized = system.serialize(toSerialize);
    var deserialized = system.deserialize(serialized);

    if (typeof(toSerialize) !== typeof(deserialized))
        failed('error in funcWithFields.  did not retain type information');
    
    var sameFields = compareObjs(toSerialize,deserialized,'funcWithFields');
    if (sameFields)
    {
        for (var s=0; s < 10; ++s)
        {
            var lhs = Math.random();
            var rhs = Math.random();

            if (toSerialize(lhs,rhs) !== deserialized(lhs,rhs))
                failed('error in funcWithFields.  Two functions did not return same value.');
        }
    }
}

function prototypeObject()
{
    function SomeConstructor()
    {
        this.a = '1';
        this.b = '2';
    }
    SomeConstructor.prototype.someFunc = function(a,b)
    {
        return a+b;
    };
    SomeConstructor.prototype.c = 5;
    SomeConstructor.prototype.d = true;
    SomeConstructor.prototype.e = {
        '1':'3'
    };

    var toSerialize = new SomeConstructor();
    var serialized = system.serialize(toSerialize);
    var deserialized = system.deserialize(serialized);
    

    if (typeof(deserialized) != 'object')
        failed('error serializing prototypeObject.  did not retain type information.');

    var sameFields= compareObjs(toSerialize,deserialized,'prototypeObject');
    if (sameFields)
    {
        for (var s =0; s < 10; ++s)
        {
            var lhs = Math.random();
            var rhs = Math.random();
            if (toSerialize.someFunc(lhs,rhs) !== deserialized.someFunc(lhs,rhs))
                failed('error serializing prototypeobject.  function did not give same values.');
        }
    }
}


//tests to ensure that when we serialize an object with a bunch of
//prototypes, we re-assemble the prototypes in the correct order.
function prototypeChain()
{
    var a  = {  };
    var b = { 'field': 'b' };
    var c = { 'field': 'c' };

    a.__proto__ = b;
    b.__proto__ = c;
    c.__proto__ = {}; //default object


    var serialized = system.serialize(a);
    var deserialized = system.deserialize(serialized);
    if (deserialized.field !== a.field)
        failed('error serializing and deserializng chained prototypes.');
}

function undefinedObject()
{
    var toSerialize  = undefined;
    var serialized   = system.serialize(toSerialize);
    var deserialized = system.deserialize(serialized);

    if (typeof(deserialized) !== 'undefined')
        failed('error when serializing and deserializing undefined');
}

function nullObject()
{
    var toSerialize  = null;
    var serialized   = system.serialize(toSerialize);
    var deserialized = system.deserialize(serialized);

    if (deserialized !== null)
        failed('error when serializing and deserializing null');
}



//checks that both original and deserialized have the same number of
//fields. and their objects have the same number of fields.
//for value types, also ensures that value types are strictly equal.
//returns true if checks pass, false if they fail.
function compareObjs(original,deserialized,test)
{
    return (compareObjsUni(original,deserialized,test,[]) &&  compareObjsUni(deserialized,original,test,[]));
}

function compareObjsUni(lhs,rhs,test,markedObjArray)
{
    var same = true;
    for (var s in rhs)
    {

        if (!(s in lhs))
        {
            same=false;
            failed(test + ' lhs did not contain field ' + s.toString());
            continue;
        }
        else if(typeof(lhs[s]) !== typeof(rhs[s]))
        {
            same = false;
            failed(test + ' mismatched types on field ' + s.toString());
            continue;
        }
        
        if ((typeof(lhs[s]) == 'object') || (typeof(lhs[s]) == 'function'))
        {
            var comparedAlready = false;
            for (var t in markedObjArray)
            {
                if (markedObjArray[t] ===lhs[s])
                    comparedAlready = true;
            }
            if (! comparedAlready)
            {
                markedObjArray.push(lhs[s]);
                same = (same && compareObjsUni(lhs[s],rhs[s],test,markedObjArray)); 
            }
        }
        else
        {
            //means that two fields should be equal
            if (lhs[s] !== rhs[s])
            {
                failed(test + ' unequal fields.  lhs: ' + lhs[s].toString() + ' rhs: ' + rhs[s].toString());
                same = false;
            }
        }
    }
    return same;
}
