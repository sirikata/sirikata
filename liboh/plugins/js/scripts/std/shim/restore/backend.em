if (typeof(std.persist) === 'undefined')
    throw 'Error.  Do not import this file directly.  Only import it from persistService.em';


if (typeof(std) === "undefined") /** @namespace */ std = {};



/**
 Clears anything that's already stored in the entry prependKey
 */
function ObjectWriter(entryName)
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
            system.storageWrite(this.mEntryName,id,serRec);
        }

        system.storageCommit(cb);
        this.recsToWrite = [];
    };

    this.addRecord = function(rec)
    {
        this.recsToWrite.push(rec);
    };
}

function readObject(entryName,itemName)
{
    var val = system.storageRead(entryName,itemName.toString());
    if (val == null)
        throw 'Erorr in read.  No record pointed to with those names';

    var returner = system.deserialize(val);
    return returner;
}
