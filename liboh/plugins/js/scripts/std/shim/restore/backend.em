if (typeof(std.persist) === 'undefined')
    throw 'Error.  Do not import this file directly.  Only import it from persistService.em';


if (typeof(std) === "undefined") /** @namespace */ std = {};


(function() {

     var keyName = function(name, subname) {
         // Generate a key name for a sub-part of this object graph.
         return name + '.' + subname;
     };

/**
 Clears anything that's already stored in the entry prependKey
 */
ObjectWriter = function(entryName)
{
    this.mEntryName = entryName;
    this.recsToWrite = [];

    this.flush = function(cb)
    {
        system.storageBeginTransaction();
        for (var s in this.recsToWrite)
        {
            var id  = this.recsToWrite[s].getID();
            var rec = this.recsToWrite[s].generateRecordObject();
            var serRec = system.serialize(rec);
            system.storageWrite(keyName(this.mEntryName, id),serRec);
        }

        system.storageCommit(cb);
        this.recsToWrite = [];
    };

    this.addRecord = function(rec)
    {
        this.recsToWrite.push(rec);
    };
};

readObject = function(entryName,itemName)
{
    var val = system.storageRead(keyName(entryName,itemName.toString()));
    if (val == null)
        throw 'Erorr in read.  No record pointed to with those names';

    var returner = system.deserialize(val);
    return returner;
};

})();