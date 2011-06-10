if (typeof(std.persist) === 'undefined')
    throw 'Error.  Do not import this file directly.  Only import it from persistService.em';



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
            throw 'Error.  Asking to mark a non-object: '+ objToMark.toString();            
        
        var name = nameService.insertObject(objToMark);
        return name;
    };


     /**
      Will run through all the fields in dataObj and attach them to this record.
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
             throw 'Error processing vec3.  First argument passed in must be a vec3';

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
             throw 'Error processing quat.  First argument passed in must be a quat';

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
           throw 'Error processing visible.  First arg passed in must be a visible.';


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
           throw 'Error processing presence.  First arg passed in must be a presence.';


       var allData = pres.getAllData();
       var record = new std.persist.Record(pres,nameService);
       //runs all data field
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
             throw 'Error processing function.  First arg passed in must be a function.';

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
    */
    var interHandler = function (objGraphCatalog,field,nameService,backendWrite,interFunc)
    {
       //check if got null object.
       if (objGraphCatalog[field] == null)
           return std.persist.wrapPropValPair(field,null);

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
        throw 'Checkpoint entire object does not work now.  Try again another time.';
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
    @param {optional} filename. String representing the name of the file to
    save this persistent object into.
    */
    std.persist.checkpointPartialPersist = function (objToPersistFrom, keyName)
    {
        if ((objToPersistFrom == null) || (typeof(objToPersistFrom) != 'object'))
            throw 'Error.  Can only checkpoint objects.';

        var backendWrite = new ObjectWriter(keyName);


       //after call to markAndBranch, name service will contain all
       //objects that were reachable from the root object
       //objToPersistFrom.
       var nameService = new std.persist.NameService();

       //add root object to nameService
       mark(objToPersistFrom, nameService);
       //recursively traverse objet graph.
       markAndBranch(objToPersistFrom,nameService,backendWrite,interHandler);

       backendWrite.flush();
       return nameService;
   };
}
)();



   // /**
   //  Called to tree-ify system object.
   //  @param objGraphCatalog This is a system object that has not already been added to markedObjects.  Must copy an entry for it into the shadow tree.  Know it is not a non-restorable.
   //  @param shadowTree: This is the serialized version of the object graph.  We progressively add to it.
   //  @param markedObjects is a map of objects that we have already cataloged.  Index of map is the object.  Points to the id of that object.
   //  */
   // function processSystem(objGraphCatalog, shadowTree, markedObjects,nameService)
   // {
   //     throw 'Error with sys object since change';

   //     if (!std.persist.checkSystem(objGraphCatalog))
   //         throw 'Error in process system.  First argument must be a system object.';

   //     throw 'Error.  Will need to change how values are pushed to shadowTree for system.';

   //     //set a type for shadowTree (it already has an id)
   //     shadowTree[std.persist.TYPE_FIELD_STRING] = std.persist.SYSTEM_TYPE_STRING;

   //     var sysData = new std.persist.NonRestorable();
   //     sysData.selfMap = objGraphCatalog.getAllData();

   //     shadowTree.selfMap = new std.persist.NonRestorable();
   //     var theMark = checkMarked(sysData.selfMap,markedObjects);
   //     if (theMark == null)
   //     {
   //         //not marked
   //         shadowTree.selfMap[std.persist.ID_FIELD_STRING] =  mark(sysData.selfMap, markedObjects);
   //         shadowTree.selfMap[std.persist.TYPE_FIELD_STRING] = std.persist.SELF_MAP_OBJECT_STRING;

   //         //go down each branch
   //         processSelfMap(sysData.selfMap,shadowTree.selfMap,markedObjects,nameService);
   //     }
   //     else
   //     {
   //         //is marked.  create pointer for it.
   //         shadowTree.selfMap = new std.persist.NonRestorable();
   //         shadowTree.selfMap[std.persist.TYPE_FIELD_STRING] = std.persist.POINTER_OBJECT_TYPE_STRING;
   //         shadowTree.selfMap[std.persist.POINTER_FIELD_STRING] = theMark;
   //     }
   // }

   // /**
   //  Called to tree-ify selfMap.
   //  @param sMap should be the selfMap pointed at by system.
   //  @see processSystem
   //  */
   // function processSelfMap(sMap,shadowTree,markedObjects,nameService)
   // {
   //     throw 'Error with self map since change';
   //     for (s in sMap)
   //     {
   //         if (s == system.__NULL_TOKEN__)
   //             continue;

   //         interHandler(sMap,shadowTree,s,markedObjects,nameService);
   //     }
   // }

   // /**
   //  Called to tree-ify a presence entry object.
   //  @param pEntry should be a PresenceEntry.  If it is not, throws exception.
   //  @see processSystem
   //  */
   // function  processPresenceEntry(pEntry,shadowTree, markedObjects,nameService)
   // {
   //     throw 'Error with presenceEntry since change';

   //     if (std.persist.checkPresenceEntry(pEntry))
   //         throw 'Error processing presence entry.  First arg passed in must be a presence entry';

   //     throw 'Error.  Will need to change how values are pushed to shadowTree for presenceEntries.';

   //     shadowTree[std.persist.TYPE_FIELD_STRING] = std.persist.PRESENCE_ENTRY_OBJECT_STRING;

   //     shadowTree.sporef  = pEntry.sporef;

   //     interHandler(pEntry, shadowTree,'presObj',markedObjects,nameService);
   //     interHandler(pEntry, shadowTree,'proxResultSet',markedObjects,nameService);
   //     interHandler(pEntry, shadowTree,'proxAddCB',markedObjects,nameService);
   //     interHandler(pEntry, shadowTree,'proxRemCB',markedObjects,nameService);
   // }



   // /**
   //  Called to tree-ify a function object.  Write now, not capturing the closure, just calling toString on function.  Ugh.
   //  @see processSystem
   //  */
   // function processFunction(func,shadowTree,markedObjects,nameService)
   // {
   //     throw 'Error with function since change';

   //     if (! std.persist.checkFunctionObject(func))
   //         throw 'Error processing function.  First arg passed in must be a function';

   //     throw 'Error.  Will need to change how values are pushed to shadowTree for functions.';

   //     shadowTree[std.persist.TYPE_FIELD_STRING] = std.persist.FUNCTION_OBJECT_TYPE_STRING;
   //     shadowTree.funcAsString  = func.toString();
   // }


   // /**
   //  Called to tree-ify a presence object.
   //   @see processSystem
   //  */
   // function processPresence(pres,shadowTree,markedObjects,nameService)
   // {
   //     throw 'Error with processPresence since change';

   //     if (! std.persist.checkPresence(pres))
   //         throw 'Error processing presence.  First arg passed in must be a presence.';

   //     throw 'Error.  Will need to change how values are pushed to shadowTree for presences.';


   //     shadowTree[std.persist.TYPE_FIELD_STRING] = std.persist.PRESENCE_OBJECT_TYPE_STRING;

   //     //loading presence data into a non-restorable so that
   //     //won't try walking subsequent data when restoring.
   //     var tmp = new std.persist.NonRestorable();
   //     tmp.data = pres.getAllData();

   //     //Tree-ifying presence data.
   //     shadowTree.sporef  = tmp.data.sporef;
   //     shadowTree.pos     = tmp.data.pos;
   //     shadowTree.vel     = tmp.data.vel;
   //     shadowTree.posTime = tmp.data.posTime;
   //     shadowTree.orient  = tmp.data.orient;
   //     shadowTree.orientVel = tmp.data.orientVel;
   //     shadowTree.orientTime = tmp.data.orientTime;
   //     shadowTree.mesh = tmp.data.mesh;
   //     shadowTree.scale = tmp.data.scale;
   //     shadowTree.isCleared = tmp.data.isCleared;
   //     shadowTree.contextId = tmp.data.contextId;
   //     shadowTree.isConnected = tmp.data.isConnected;

   //     interHandler(tmp.data,shadowTree, 'connectedCallback', markedObjects,nameService);
   //     shadowTree.isSuspended = tmp.data.isSuspended;
   //     shadowTree.suspendedVelocity = tmp.data.suspendedVelocity;
   //     shadowTree.suspendedOrientationVelocity = tmp.data.suspendedOrientationVelocity;
   //     interHandler(tmp.data,shadowTree, 'proxAddedCallback', markedObjects,nameService);
   //     interHandler(tmp.data,shadowTree, 'proxRemovedCallback', markedObjects,nameService);
   // }


   // /**
   //  Called to tree-ify a timer object
   //  */
   // function processTimer(timer,shadowTree,markedObjects,nameService)
   // {
   //     throw 'Error with processTimer since change';

   //     if (! std.persist.checkTimer(timer))
   //         throw 'Error processing timer.  First arg passed in must be a timer.';

   //     throw 'Error.  Will need to change how values are pushed to shadowTree for timers.';

   //     shadowTree[std.persist.TYPE_FIELD_STRING] = std.persist.TIMER_OBJECT_STRING;

   //     //loading presence data into a non-restorable so that
   //     //won't try walking subsequent data when restoring.
   //     var tmp  = new std.persist.NonRestorable();
   //     tmp.data = timer.getAllData();
   //     shadowTree.isCleared = tmp.data.isCleared;
   //     shadowTree.contextId = tmp.data.contextId;
   //     shadowTree.period = shadowTree.isCleared ? null : tmp.data.period;
   //     shadowTree.timeRemaining = shadowTree.isCleared ? null : tmp.data.timeRemaining;
   //     shadowTree.isSuspended = shadowTree.isCleared ? null : tmp.data.isSuspended;

   //     if (shadowTree.isCleared)
   //         shadowTree.callback = null;
   //     else
   //         interHandler(tmp.data,shadowTree,'callback',markedObjects,nameService);

   // }



   // /**
   //  Called to tree-ify a visible object
   //  */
   // function processVisible(vis, shadowTree,markedObjects,nameService)
   // {
   //     throw 'Error with processVisible since change';

   //     if (! std.persist.checkVisible(vis))
   //         throw 'Error processing visible.  First argument passed in must be a visible.';

   //     throw 'Error.  Will need to change how values are pushed to shadowTree for visibles.';

   //     shadowTree[std.persist.TYPE_FIELD_STRING] = std.persist.VISIBLE_OBJECT_STRING;

   //     //loading presence data into a non-restorable so that
   //     //won't try walking subsequent data when restoring.
   //     var tmp = new std.persist.NonRestorable();
   //     tmp.data = pres.getAllData();

   //     for (var s in tmp.data)
   //         shadowTree[s] = tmp.data[s];
   // }



