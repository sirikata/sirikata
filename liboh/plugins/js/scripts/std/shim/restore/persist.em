if (typeof(std.persist) === 'undefined')
    throw 'Error.  Do not import this file directly.  Only import it from persistService.em';



(function ()
 {


     /* Type constants */
     //use this as an index to retrieve the type of local objects
     var GET_TYPE_STRING                   =            '__getType';
    

     //set the type field of an object to this if have a loop.
     var SYSTEM_TYPE_STRING                =               'system';
     var PRESENCE_OBJECT_TYPE_STRING       =             'presence';
     var VISIBLE_TYPE_STRING               =              'visible';
     var TIMER_TYPE_STRING                 =                'timer';
     var PRESENCE_ENTRY_TYPE_STRING        =        'presenceEntry';
     var FUNCTION_OBJECT_TYPE_STRING       =   'funcObjectAsString';
     
    

    
    /**
     @param obj to check if it is a system object
     @return true if system object.  false otherwise
     */
    var checkSystem = function (obj)
    {
        if (GET_TYPE_STRING in obj)
            return (obj[GET_TYPE_STRING]() == SYSTEM_TYPE_STRING);
        
        return false;
    };

    /**
     @param obj to check if it is a presence object
     @return true if presence object.  false otherwise
     */
    var checkPresence = function (obj)
    {
        if (GET_TYPE_STRING in obj)
            return (obj[GET_TYPE_STRING]() == PRESENCE_TYPE_STRING);
        
        return false;
    };

    /**
     @param obj to check if it is a visible object
     @return true if visible object.  false otherwise
     */
    var checkVisible = function (obj)
    {
        if (GET_TYPE_STRING in obj)
            return (obj[GET_TYPE_STRING]() == VISIBLE_TYPE_STRING);
        
        return false;
    };

    /**
     @param obj to check if it is a timer object
     @return true if timer object.  false otherwise
     */
    var checkTimer = function (obj)
    {
        if (GET_TYPE_STRING in obj)
            return (obj[GET_TYPE_STRING]() == TIMER_TYPE_STRING);

        return false;
    };

    
    /**
     @param obj to check if it is a PresenceEntry object
     @return true if PresenceEntry object.  false otherwise
     */
    var checkPresenceEntry = function (obj)
    {
        if (GET_TYPE_STRING in obj)
            return (obj[GET_TYPE_STRING]() == PRESENCE_ENTRY_TYPE_STRING);
        
        return false;
    };


    
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
        if (typeof(objToMark) != 'object')
            throw 'Error.  Asking to mark a non-object: '+objToMark.toString();            
        
        var name = nameService.insertObject(objToMark);
        return name;
    };


    /**
     @param objGraphCatalog This is the root of the object graph that we have not yet copied to shadowTree
     @param markedObjects is a map of objects that we have already cataloged.  Keys are object references.  Values are unique names associated with each object given by nameService.
     */
    var markAndBranch= function(objGraphCatalog, nameService,backendWrite,interFunc)
    {
        var record = new std.persist.Record(objGraphCatalog,nameService);

        for (var s in objGraphCatalog)
        {
            // check typeof s;
            // lkjs;
            if (typeof(objGraphCatalog[s]) != 'object')
                record.pushValueType(std.persist.wrapPropValPair(s,objGraphCatalog[s]));
            else
                record.pushObjType(interFunc(objGraphCatalog,s,nameService,backendWrite,interFunc));

        }
        backendWrite.addRecord(record);
    };


   /**
    @param {object} objGraphCatalog the local copy of the object whose field we're trying to persist.
    @param field: the name of the field of the local copy of the object we're trying to persist.
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



