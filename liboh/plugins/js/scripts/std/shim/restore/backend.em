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

var readObjectCallback = function(keyname, cb, success, rs) {
    if (!success) cb(false);

    var returner = system.deserialize( rs[keyname]) ;
    cb(true, returner);
};
readObject = function(entryName, itemName, cb)
{
    var keyname = keyName(entryName,itemName.toString());
    var wrapped_cb = undefined;
    if (cb)
        wrapped_cb = std.core.bind(readObjectCallback, undefined, keyname, cb);
    var queued = system.storageRead(keyname, wrapped_cb);
    if (!queued)
        throw 'Error queuing read request.';
};

})();