if (typeof(std.persist) === 'undefined')
    throw 'Error.  Do not import this file directly.  Only import it from persistService.em';


if (typeof(std) === "undefined") /** @namespace */ std = {};



std.persist.BackendService = function(prependkey)
{
    var mPrependKey = prependKey;

    var sequenceKey = null;
    var recsToWrite = [];

    this.writeSequence = function(keyName)
    {
        sequenceKey = system.backendCreateEntry(prependKey+keyName);
    };


    this.flush = function()
    {
        for (var s in recsToWrite)
        {
            var id  = recsToWrite[s].getID();
            var rec = recsToWrite[s].generateRecordObject();
            var serRec = system.serialize(rec);
            system.backendWrite(sequenceKey,id,serRec);
        }
        
        system.backendFlush(sequenceKey);
        sequenceKey = null;
        recsToWrite = [];
    };

    this.addRecord = function(rec)
    {
        recsToWrite.push(rec);
    };

    this.read = function (keyName,ptrId)
    {
        var val = system.backendRead(prependKey+keyName,ptrId);
        if (val == null)
            throw 'Erorr in read.  No record pointed to with those names';

        
        var returner = system.deserialize(val);
        return returner;
    };
};

