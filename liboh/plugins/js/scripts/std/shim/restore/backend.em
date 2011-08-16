if (typeof(std.persist) === 'undefined')
    throw new Error('Error.  Do not import this file directly.  Only import it from persistService.em');


if (typeof(std) === "undefined") /** @namespace */ std = {};


(function() {

     var keyName = function(name, subname) {
         // Generate a key name for a sub-part of this object graph.
         return name + '.' + subname;
     };

     /**
      Clears anything that's already stored in the entry prependKey

      @param {bool} queueTransactions If true, then don't create a new
      transaction and commit it after calling flush. Just return.
      
      */
     ObjectWriter = function(entryName,queueTransactions)
     {
         this.mEntryName = entryName;
         this.recsToWrite = [];
         this.queueTransactions = queueTransactions;
     };


     /**
      @param {function} cb If ObjectWriter is not in queueTransactions
      mode (ie, this.queueTransactions is false), then will begin a
      transaction and commit that transaction, passing cb into
      system.storageCommit.  If in queueTransactions mode, then does
      not start a transaction or commit it, but after flush is
      finished, calls cb directly, passing no args.
      */
     ObjectWriter.prototype.flush = function (cb)
     {
         if (! this.queueTransactions)
             system.storageBeginTransaction();
         
         for (var s in this.recsToWrite)
         {
             var entryName = this.recsToWrite[s][0];
             var id  = this.recsToWrite[s][1].getID();
             var rec = this.recsToWrite[s][1].generateRecordObject();
             var serRec = system.serialize(rec);
             system.storageWrite(keyName(entryName, id),serRec);
         }

         this.recsToWrite = [];
         if (! this.queueTransactions)
             system.storageCommit(cb);
         else
             cb();
     };
     
     ObjectWriter.prototype.addRecord = function(rec)
     {
        this.recsToWrite.push([this.mEntryName,rec]);
     };

     ObjectWriter.prototype.changeEntryName = function (newEntryName)
     {
        this.mEntryName = newEntryName;         
     };

     
var readObjectCallback = function(keyname, cb, success, rs) {
    if (!success)
    {
        cb(false);
        return;
    }
    
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
        throw new Error('Error queuing read request.');
};

})();