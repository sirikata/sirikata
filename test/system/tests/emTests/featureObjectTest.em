//Each presence adds a feature named newFeature, which starts with value 1.
//Periodically, we check the feature objects of the other

//Each counts up from 1-5.  Then delete 

system.require('helperLibs/util.em');
system.require('std/core/repeatingTimer.em');

var VALUE_FIELD_NAME    = 'val';
var OTHER_FIELD_NAME    = 'val2';
var OTHER_FIELD_VAL     = 'deleted';
var mValue              = 0;
var otherValue          = -1;
var END_VALUE           = 5;
var gotIntoCheckDelete  = false;
var TOTAL_TIMEOUT       = 7;

mTest = new UnitTest('featureObjectTest');

system.onPresenceConnected(
    function()
    {
        system.self.setQueryAngle(.0001);
        std.core.RepeatingTimer(.3,checkFeatures);
        std.featureObject.addField(
            VALUE_FIELD_NAME,mValue.toString(),system.self);
    });

killAfterTime(TOTAL_TIMEOUT,checkSuccessFail);



//every so often, look through entire prox set to see if got an update
function checkFeatures(repTimer)
{
    if ((mValue >= END_VALUE)
        || (otherValue >= END_VALUE))
    {
        repTimer.suspend();
        tryDelete();
        return;            
    }
    
    var proxSet = system.getProxSet(system.self);
    for (var s in proxSet)
    {
        //we don't care about our own features.
        if (s == system.self.toString())
            continue;
        
        var featObj = proxSet[s].featureObject;
        if (typeof(featObj) != 'undefined')
        {
            var featVal = featObj[VALUE_FIELD_NAME];
            if (typeof(featVal) != 'undefined')
            {
                featVal = parseInt(featVal);
                if (featVal !== otherValue)
                {
                    mValue = featVal + 1;
                    std.featureObject.changeField(
                        VALUE_FIELD_NAME,mValue.toString(),system.self);
                    otherValue = featVal;
                }
            }
        }
    }
}

function tryDelete()
{
    std.featureObject.addField(OTHER_FIELD_NAME,OTHER_FIELD_VAL,system.self);
    std.featureObject.removeField(VALUE_FIELD_NAME, system.self);
    system.timeout(1,checkDelete);
}

function checkDelete()
{
    var proxSet = system.getProxSet(system.self);
    var gotSomeProxes = false;
    for (var s in proxSet)
    {
        if (s == system.self.toString())
            continue;

        gotSomeProxes = true;
        
        var featObj = proxSet[s].featureObject;

        if (typeof(featObj[VALUE_FIELD_NAME]) != 'undefined')
            mTest.fail('Could not delete value field');

        var otherField = featObj[OTHER_FIELD_NAME];
        if (typeof(otherField) == 'undefined')
            mTest.fail('Could not read other field');
        else if(otherField !== OTHER_FIELD_VAL )
            mTest.fail('Incorrect value in otherField');
    }
    if (!gotSomeProxes)
        mTest.fail('Never got to read any visibles in checkDelete');
    
    gotIntoCheckDelete = true;
}


function checkSuccessFail()
{
    if (!gotIntoCheckDelete)
        mTest.fail('Never tried to delete data');

    if (!mTest.hasFailed)
        mTest.success('Got all the way through.');
}