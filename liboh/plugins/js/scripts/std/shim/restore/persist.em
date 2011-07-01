if (typeof(std.persist) === 'undefined')
    throw new Error('Error.  Do not import this file directly.  Only import it from persistService.em');



(function ()
 {
    
   /**
    @param objToMark if it's an object (and not null), then it logs the
    object as reachable from the object that we're persisting (ie, puts
    it in markedObjects map).

    @exception Throws an exception if the object that we're supposed to
    mark has already been marked.

    @return returns the unique name given by nameService of objToMark.
    Can use this unique name to index into the backend for that object.
    Also, can use the name to get back the initial object reference later
    (through nameService).

    */
    var mark = function(objToMark,nameService)
    {
        if ((typeof(objToMark) != 'object') && (typeof(objToMark) != 'function'))            
            throw new Error('Error.  Asking to mark a non-object: '+ objToMark.toString());
        
        var name = nameService.insertObject(objToMark);
        return name;
    };


     var rootSpecial = false;

     /**
      Will run through all the fields in dataObj and attach them to this record.
      @see interHandler for description of alias.
      */
     function runFields(dataObj,record,nameService,backendWrite,interFunc)
     {
         for (var s in dataObj)
         {
            if ((typeof(dataObj[s]) == 'object') || (typeof(dataObj[s]) == 'function'))
                record.pushObjType(interFunc(dataObj,s,nameService,backendWrite,interFunc));
            else  //assumes it's a value type
                record.pushValueType(std.persist.wrapPropValPair(s,dataObj[s]));
         }
     }

     /**
      Called to tree-ify a vec3 object.
      @see processSystem
      */
     function processVec3(v3,nameService,backendWrite,interFunc)
     {
         if (! std.persist.checkVec3(v3))
             throw new Error('Error processing vec3.  First argument passed in must be a vec3');

         var record = new std.persist.Record(v3,nameService);
         record.pushValueType(std.persist.wrapPropValPair('x',v3.x));
         record.pushValueType(std.persist.wrapPropValPair('y',v3.y));
         record.pushValueType(std.persist.wrapPropValPair('z',v3.z));
         backendWrite.addRecord(record);
     }

     /**
      Called to tree-ify a quaternion object.
      @see processSystem
      */
     function processQuat(quat,nameService,backendWrite,interFunc)
     {
         if (! std.persist.checkQuat(quat))
             throw new Error('Error processing quat.  First argument passed in must be a quat');

         var record = new std.persist.Record(quat,nameService);
         record.pushValueType(std.persist.wrapPropValPair('x',quat.x));
         record.pushValueType(std.persist.wrapPropValPair('y',quat.y));
         record.pushValueType(std.persist.wrapPropValPair('z',quat.z));
         record.pushValueType(std.persist.wrapPropValPair('w',quat.w));
         backendWrite.addRecord(record);
     }


   /**
    Called to tree-ify a visible object.
     @see processSystem
    */
   function processVisible(vis,nameService,backendWrite,interFunc)
   {
       if (! std.persist.checkVisible(vis))
           throw new Error('Error processing visible.  First arg passed in must be a visible.');


       var allData = vis.getAllData();
       var record = new std.persist.Record(vis,nameService);
       //runs all data field
       runFields(allData,record,nameService,backendWrite,interFunc);
       backendWrite.addRecord(record);
   }
     


   /**
    Called to tree-ify a presence object.
     @see processSystem
    */
   function processPresence(pres,nameService,backendWrite,interFunc)
   {
       if (! std.persist.checkPresence(pres))
           throw new Error('Error processing presence.  First arg passed in must be a presence.');

       var allData = pres.getAllData();
       var record = new std.persist.Record(pres,nameService);

       //runs all data field
       //using the pres alias.
       runFields(allData,record,nameService,backendWrite,interFunc);
       backendWrite.addRecord(record);
   }

     /**
      Will need to fix this later to actually capture the closure.
      For now, just calls toString on function, and hopes for the
      best.
      */
     function processFunction(func,nameService,backendWrite,interFunc)
     {
         if (! std.persist.checkFunction(func))
             throw new Error('Error processing function.  First arg passed in must be a function.');

         //for now, just pushing the text of the function to the backend record.
         var record = new std.persist.Record(func,nameService);
         record.pushValueType(std.persist.wrapPropValPair('funcField',func.toString()));
         backendWrite.addRecord(record);
     }
     
     
    /**
     @param objGraphCatalog This is the root of the object graph that
     we have not yet copied to shadowTree
     
     @param markedObjects is a map of objects that we have already
     cataloged.  Keys are object references.  Values are unique names
     associated with each object given by nameService.
     */
    var markAndBranch= function(objGraphCatalog, nameService,backendWrite,interFunc)
    {
        if (std.persist.checkPresence (objGraphCatalog))
        {
            processPresence(objGraphCatalog,nameService,backendWrite,interFunc);
            return;
        }
        if (std.persist.checkFunction(objGraphCatalog))
        {
            processFunction(objGraphCatalog,nameService,backendWrite,interFunc);
            return;
        }
        if (std.persist.checkVec3(objGraphCatalog))
        {
            processVec3(objGraphCatalog,nameService,backendWrite,interFunc);
            return;
        }
        if (std.persist.checkQuat(objGraphCatalog))
        {
            processQuat(objGraphCatalog,nameService,backendWrite,interFunc);
            return;
        }
        if (std.persist.checkVisible(objGraphCatalog))
        {
            processVisible(objGraphCatalog,nameService,backendWrite,interFunc);
            return;
        }
        

        //actually runs through all the fields of the objGraphCatalog
        var record = new std.persist.Record(objGraphCatalog,nameService);
        runFields(objGraphCatalog,record,nameService,backendWrite,interFunc);
        backendWrite.addRecord(record);
    };


   /**
    @param {object} objGraphCatalog the local copy of the object whose
    field we're trying to persist.

    @param field: the name of the field of the local copy of the
    object we're trying to persist.

    @param {object} alias (optional).  For many of the special
    objects, (eg. presences, vecs, quaternions, etc.), instead of
    saving the actual object, we frequently use its alldata values
    instead of the object itself for serialization.  This means that
    the first object that we mark sometimes may not get written to
    storage as 0 (instead, it gets written as 1,2,3, etc.).  If alias
    is provided, we use the mark provided by the name service of the
    alias instead of the object itself.
    */
    var interHandler = function (objGraphCatalog,field,nameService,backendWrite,interFunc,alias)
    {
       //check if got null object.
       if (objGraphCatalog[field] == null)
           return std.persist.wrapPropValPair(field,null);

        //use the alias of this object instead of the object itself.
        // if (typeof(alias) !== 'undefined')
        // {
        //     //we assume that all aliased objects are already marked.
        //     var theMark = nameService.lookupName(alias);
        //     return std.persist.wrapPropValPair(field,theMark);
        // }

        
       var theMark = nameService.lookupName(objGraphCatalog[field]);
       if (theMark == nameService.DNE)
       {
           //may want to do special things here to determine if it is a special object;
           //means that we have not already tagged this object
           theMark =  mark(objGraphCatalog[field],nameService);
           markAndBranch(objGraphCatalog[field],nameService,backendWrite,interFunc);
       }
       return std.persist.wrapPropValPair(field,theMark);
    };

    
    std.persist.checkpoint =     function (presenceEntries)
    {
        throw new Error('Checkpoint entire object does not work now.  Try again another time.');
        var globalObj = (function()
                         {
                             return this;
                         })();
        
        std.persist.checkpointPartialPersist(globalObj);
    };


     
     
     
   /**
    Currently, only call checkpointPartialPersist on objects that do not
    point back to global object.  Call toString on functions, and aren't
    restoring them correctly.

    @param objToPersistFrom Must be non-null object.  Creates a
    persistent subtree of this object.
    @param {optional} filename String representing the name of the file to
    save this persistent object into.

    @param {optional} cb Callback of the form function(bool success)
           which is invoked when operation completes.
    */
    std.persist.checkpointPartialPersist = function (objToPersistFrom, keyName, cb)
    {
        std.persist.persistMany([[objToPersistFrom,keyName]],cb);
   };



     /**
      @param {array} arrayOfObjKeyNames An array.  Each element in the
      array has two fields, the first is an object that will be
      persisted.  The second is a keyName used to access that object.

      @param {optional} cb @see checkpointParitalPersist.
      */
     std.persist.persistMany = function (arrayOfObjKeyNames,cb)
     {
         var backendWrite = new ObjectWriter('default');

         for (var s in arrayOfObjKeyNames)
         {
             //after call to markAndBranch, name service will contain all
             //objects that were reachable from the root object
             //objToPersistFrom.
             var nameService = new std.persist.NameService();
             
             if (typeof(arrayOfObjKeyNames[s]['length']) != 'number')
                 throw new Error('Error.  Require an array to be passed in to persist many');
             
             
             backendWrite.changeEntryName(arrayOfObjKeyNames[s][1]);
             var objToPersistFrom = arrayOfObjKeyNames[s][0];

             if ((objToPersistFrom == null) || (typeof(objToPersistFrom) != 'object'))
                 throw new Error('Error.  Can only checkpoint objects.');
             
             //add root object to nameService...if it's not a special
             //object.  Hack-ish.  Need to get default entry to point at 0.
             mark(objToPersistFrom, nameService);

             
             //recursively traverse objet graph.
             markAndBranch(objToPersistFrom,nameService,backendWrite,interHandler);
         }
         backendWrite.flush(cb);
     };


     
}
)();
