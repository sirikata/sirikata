

if (typeof(std.persist) === 'undefined')
    throw 'Error.  Do not import this file directly.  Only import it from persistService.em';



//separate, isolating namespace to prevent global namespace pollution
(function ()
{

    /**
     @param {Object} Object to get record type of.
     @return {String} One of fixed number of type vals (declared in
     persistService to use to indicate type of object, eg. presence,
     system, reg ob, etc.)
     */
    function getRecordType(objToGetTypeOf)
    {
        if ((typeof(objToGetTypeOf) != 'object') && (typeof(objToGetTypeOf) != 'function'))
            throw 'Error in getRecordType.  Argument passed was not object or func';

        if (std.persist.checkFunction(objToGetTypeOf))
            return std.persist.FUNCTION_OBJECT_TYPE_STRING;
        
        if (std.persist.checkPresence(objToGetTypeOf))
            return std.persist.PRESENCE_OBJECT_TYPE_STRING;

        if (std.persist.checkVec3(objToGetTypeOf))
            return std.persist.VEC_TYPE_STRING;

        if (std.persist.checkQuat(objToGetTypeOf))
            return std.persist.QUAT_TYPE_STRING;
        
        
        return std.persist.BASIC_OBJECT_TYPE_STRING;
    }
    
/*Create a separate namespace for name service;*/
/**
 @param {object} The object that is being logged by this record.
 Note, depending on the typ of object, eg. presence, regular object,
 system object, etc., we will use different values for recordType.

 @param {nameService} The name service used to register the objects.
 Ensures that each object has a unique id.
 */
std.persist.Record = function(objectRecordOf, nameService)
{
    var mID = nameService.lookupName(objectRecordOf);
    if (mID == nameService.DNE)
        throw 'Error in Record constructor.  Require object passed in to be named in nameService';

    
    var recordType = getRecordType(objectRecordOf);
    
    var valueRecords =[];
    var objRecords   =[];
    var funcRecords  =[];
    
    this.getID = function()
    {
        return mID;
    };
        
    this.pushValueType = function(pair)
    {
        valueRecords.push(pair);
    };
    
    this.pushObjType = function(pair)
    {
        objRecords.push(pair);
    };

    this.pushFuncType = function(pair)
    {
        funcRecords.push(pair);
    };
    
    
    this.generateRecordObject = function ()
    {
        var returner = { };

        returner['mID']  = mID;
        returner['type'] = recordType; 
        var index =0;
        for (var s in valueRecords)
        {
            var prop  = std.persist.getPropFromPropValPair(valueRecords[s]);
            var val   = std.persist.getValueFromPropValPair(valueRecords[s]);
            returner[index] = [prop, val.toString(), typeof(val)];
            ++index;
        }

        for (var s in objRecords)
        {
            var prop = std.persist.getPropFromPropValPair(objRecords[s]);
            var val  = std.persist.getValueFromPropValPair(objRecords[s]);
            if (val == null)
            {
                returner[index] = [prop,"null", "null"];
                ++index;
                continue;
            }
            
            //means that we've got an object to put in.
            returner[index] = [prop, val.toString(), "object"];
            ++index;
        }
        
        return returner;
    };
};

    })();