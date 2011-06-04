if (typeof(std.persist) === 'undefined')
    throw 'Error.  Do not import this file directly.  Only import it from persistService.em';


if (typeof(std) === "undefined") /** @namespace */ std = {};



function BackendService(prependKey)
{
    
    this.mPrependKey = prependKey;

    this.sequenceKey = null;
    this.recsToWrite = [];

    this.writeSequence = function(keyName)
    {
        this.sequenceKey = system.backendCreateEntry(this.mPrependKey+keyName);
    };


    this.flush = function()
    {
        for (var s in this.recsToWrite)
        {
            var id  = this.recsToWrite[s].getID();
            var rec = this.recsToWrite[s].generateRecordObject();
            var serRec = system.serialize(rec);
            system.backendWrite(this.sequenceKey,id,serRec);
        }
        
        system.backendFlush(this.sequenceKey);
        this.sequenceKey = null;
        this.recsToWrite = [];
    };

    this.addRecord = function(rec)
    {
        this.recsToWrite.push(rec);
    };

    this.read = function (keyName,ptrId)
    {
        var val = system.backendRead(this.mPrependKey+keyName,ptrId.toString());
        if (val == null)
            throw 'Erorr in read.  No record pointed to with those names';

        var returner = system.deserialize(val);
        return returner;
    };
}

std.persist.Backend = new BackendService('bftmTest');


