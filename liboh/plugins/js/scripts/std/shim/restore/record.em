

if (typeof(std.persist) === 'undefined')
    throw 'Error.  Do not import this file directly.  Only import it from persistService.em';




/*Create a separate namespace for name service;*/
std.persist.Record = function(objectRecordOf, nameService)
{
    var mID = nameService.lookupName(objectRecordOf);
    if (mID == nameService.DNE)
        throw 'Error in Record constructor.  Require object passed in to be named in nameService';

        
    var valueRecords =[];
    var objRecords   =[];

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

    this.generateRecordObject = function ()
    {
        var returner = { };

        returner['mID'] = mID;
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