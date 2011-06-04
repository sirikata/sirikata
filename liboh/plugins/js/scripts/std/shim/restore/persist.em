if (typeof(std.persist) === 'undefined')
    throw 'Error.  Do not import this file directly.  Only import it from persistService.em';



//builds a parallel tree of to restore objects.
//__markField is set to true for each object that we find on the way.
//

function checkpoint(presenceEntries)
{
    throw 'Checkpoint entire object does not work now.  Try again another time.';
    var globalObj = (function()
                  {
                      return this;
                  })();

    checkpointPartialPersist(globalObj);
}


/**
 Currently, only call checkpointPartialPersist on objects that do not
 point back to global object.  Call toString on functions, and aren't
 restoring them correctly.

 @param objToPersistFrom Must be non-null object.  Creates a
 persistent subtree of this object.
 @param {optional} filename. String representing the name of the file to
 save this persistent object into.
 */
function checkpointPartialPersist(objToPersistFrom, keyName)
{
    if ((objToPersistFrom == null) || (typeof(objToPersistFrom) != 'object'))
        throw 'Error.  Can only checkpoint objects.';
    
    std.persist.Backend.writeSequence(keyName);
    
    //after call to markAndBranch, markedObjectMap will contain all
    //objects that were reachable from the root object
    //objToPersistFrom.
    var markedObjectMap = {};
    var nameService = new std.persist.NameService();
    
    //add root object to markedObjectMap before beginning.
    mark(objToPersistFrom, markedObjectMap,nameService);
    //recursively traverse objet graph.
    markAndBranch(objToPersistFrom, markedObjectMap,nameService);

    
    //log the subtree of objects reachable from objToPersistFrom in
    //nameService;
    nameService.enterSubtreeObjects(objToPersistFrom, markedObjectMap);
    
    std.persist.Backend.flush();
    return nameService;
}


/**
 @param value/object to check if it has been marked.
 @return returns name of object if value passed in is an object that
 has been marked.  Returns null otherwise
 */
function checkMarked(objToCheck, markedObjects)
{
    return (objToCheck in markedObjects) ? markedObjects[objToCheck] : null ;
}


/**
 @param objToMark if it's an object (and not null), then it logs the
 object as reachable from the object that we're persisting (ie, puts
 it in markedObjects map).

 @param markedObjects Map.  Keys are object references and values are
 names (unique ids provided by nameService).  Contains all objects
 that are reachable from the object that we're persisting.
 
 @exception Throws an exception if the object that we're supposed to
 mark has already been marked.

 @return returns the unique name given by nameService of objToMark.
 Can use this unique name to index into the backend for that object.
 Also, can use the name to get back the initial object reference later
 (through nameService).

 */
function mark(objToMark, markedObjects,nameService)
{
    if (checkMarked(objToMark,markedObjects))
        throw 'Error.  Asking to mark object that\'s already been marked';

    if (typeof(objToMark) != 'object')
        throw 'Error.  Asking to mark a non-object: '+objToMark.toString();            

    var name = nameService.insertObject(objToMark);
    markedObjects[objToMark] = name;
    return name;
}


/**
 @param objGraphCatalog This is the root of the object graph that we have not yet copied to shadowTree
 @param markedObjects is a map of objects that we have already cataloged.  Keys are object references.  Values are unique names associated with each object given by nameService.
 */
function markAndBranch(objGraphCatalog, markedObjects,nameService)
{

    //set a type for this object;
    if (std.persist.checkSystem(objGraphCatalog))
    {
        processSystem(objGraphCatalog,markedObjects,nameService);
        return;
    }
    if (std.persist.checkPresence(objGraphCatalog))
    {
        processPresence(objGraphCatalog,markedObjects,nameService);
        return;            
    }
    if (std.persist.checkTimer(objGraphCatalog))
    {
        processTimer(objGraphCatalog,markedObjects,nameService);
        return;
    }
    if (std.persist.checkVisible(objGraphCatalog))
    {
        processVisible(objGraphCatalog,markedObjects,nameService);
        return;
    }
    if (std.persist.checkPresenceEntry(objGraphCatalog))
    {
        processPresenceEntry(objGraphCatalog,markedObjects,nameService);
        return;
    }
    if (std.persist.checkFunctionObject(objGraphCatalog))
    {
        processFunction(objGraphCatalog,markedObjects,nameService);
        return;
    }

    var record = new std.persist.Record(objGraphCatalog,nameService);
    
    for (var s in objGraphCatalog)
    {
        // check typeof s;
        // lkjs;
        if (typeof(objGraphCatalog[s]) != 'object')
            record.pushValueType(std.persist.wrapPropValPair(s,objGraphCatalog[s]));
        else
            record.pushObjType(interHandler(objGraphCatalog,s,markedObjects,nameService));
    }
    std.persist.Backend.addRecord(record);
}



/**
 @param {object} objGraphCatalog the local copy of the object whose field we're trying to persist.
 @param field: the name of the field of the local copy of the object we're trying to persist.
 */
function interHandler(objGraphCatalog,field,markedObjects,nameService)
{
    //check if got null object.
    if (objGraphCatalog[field] == null)
        return std.persist.wrapPropValPair(field,null);
    
    var theMark = checkMarked(objGraphCatalog[field],markedObjects);
    if (theMark == null)
    {
        //may want to do special things here to determine if it is a special object;
        //means that we have not already tagged this object
        theMark =  mark(objGraphCatalog[field],markedObjects,nameService);
        markAndBranch(objGraphCatalog[field],markedObjects,nameService);
    }
    return std.persist.wrapPropValPair(field,theMark);
}


/**
 Called to tree-ify system object.
 @param objGraphCatalog This is a system object that has not already been added to markedObjects.  Must copy an entry for it into the shadow tree.  Know it is not a non-restorable.
 @param shadowTree: This is the serialized version of the object graph.  We progressively add to it.
 @param markedObjects is a map of objects that we have already cataloged.  Index of map is the object.  Points to the id of that object.
 */
function processSystem(objGraphCatalog, shadowTree, markedObjects,nameService)
{
    throw 'Error with sys object since change';
    
    if (!std.persist.checkSystem(objGraphCatalog))
        throw 'Error in process system.  First argument must be a system object.';

    throw 'Error.  Will need to change how values are pushed to shadowTree for system.';
    
    //set a type for shadowTree (it already has an id)
    shadowTree[std.persist.TYPE_FIELD_STRING] = std.persist.SYSTEM_TYPE_STRING;
    
    var sysData = new std.persist.NonRestorable();
    sysData.selfMap = objGraphCatalog.getAllData();
    
    shadowTree.selfMap = new std.persist.NonRestorable();
    var theMark = checkMarked(sysData.selfMap,markedObjects);
    if (theMark == null)
    {
        //not marked
        shadowTree.selfMap[std.persist.ID_FIELD_STRING] =  mark(sysData.selfMap, markedObjects);
        shadowTree.selfMap[std.persist.TYPE_FIELD_STRING] = std.persist.SELF_MAP_OBJECT_STRING;
        
        //go down each branch
        processSelfMap(sysData.selfMap,shadowTree.selfMap,markedObjects,nameService);
    }
    else
    {
        //is marked.  create pointer for it.
        shadowTree.selfMap = new std.persist.NonRestorable();
        shadowTree.selfMap[std.persist.TYPE_FIELD_STRING] = std.persist.POINTER_OBJECT_TYPE_STRING;
        shadowTree.selfMap[std.persist.POINTER_FIELD_STRING] = theMark;
    }
}

/**
 Called to tree-ify selfMap.
 @param sMap should be the selfMap pointed at by system.
 @see processSystem
 */
function processSelfMap(sMap,shadowTree,markedObjects,nameService)
{
    throw 'Error with self map since change';
    for (s in sMap)
    {
        if (s == system.__NULL_TOKEN__)
            continue;

        interHandler(sMap,shadowTree,s,markedObjects,nameService);
    }
}

/**
 Called to tree-ify a presence entry object.
 @param pEntry should be a PresenceEntry.  If it is not, throws exception.
 @see processSystem
 */
function  processPresenceEntry(pEntry,shadowTree, markedObjects,nameService)
{
    throw 'Error with presenceEntry since change';
    
    if (std.persist.checkPresenceEntry(pEntry))
        throw 'Error processing presence entry.  First arg passed in must be a presence entry';

    throw 'Error.  Will need to change how values are pushed to shadowTree for presenceEntries.';
    
    shadowTree[std.persist.TYPE_FIELD_STRING] = std.persist.PRESENCE_ENTRY_OBJECT_STRING;
    
    shadowTree.sporef  = pEntry.sporef;

    interHandler(pEntry, shadowTree,'presObj',markedObjects,nameService);
    interHandler(pEntry, shadowTree,'proxResultSet',markedObjects,nameService);
    interHandler(pEntry, shadowTree,'proxAddCB',markedObjects,nameService);
    interHandler(pEntry, shadowTree,'proxRemCB',markedObjects,nameService);
}



/**
 Called to tree-ify a function object.  Write now, not capturing the closure, just calling toString on function.  Ugh.
 @see processSystem
 */
function processFunction(func,shadowTree,markedObjects,nameService)
{
    throw 'Error with function since change';
    
    if (! std.persist.checkFunctionObject(func))
        throw 'Error processing function.  First arg passed in must be a function';

    throw 'Error.  Will need to change how values are pushed to shadowTree for functions.';
    
    shadowTree[std.persist.TYPE_FIELD_STRING] = std.persist.FUNCTION_OBJECT_TYPE_STRING;
    shadowTree.funcAsString  = func.toString();
}
    

/**
 Called to tree-ify a presence object.
  @see processSystem
 */
function processPresence(pres,shadowTree,markedObjects,nameService)
{
    throw 'Error with processPresence since change';
    
    if (! std.persist.checkPresence(pres))
        throw 'Error processing presence.  First arg passed in must be a presence.';

    throw 'Error.  Will need to change how values are pushed to shadowTree for presences.';
    
    
    shadowTree[std.persist.TYPE_FIELD_STRING] = std.persist.PRESENCE_OBJECT_TYPE_STRING;

    //loading presence data into a non-restorable so that
    //won't try walking subsequent data when restoring.
    var tmp = new std.persist.NonRestorable();
    tmp.data = pres.getAllData();

    //Tree-ifying presence data.
    shadowTree.sporef  = tmp.data.sporef;
    shadowTree.pos     = tmp.data.pos;
    shadowTree.vel     = tmp.data.vel;
    shadowTree.posTime = tmp.data.posTime;
    shadowTree.orient  = tmp.data.orient;
    shadowTree.orientVel = tmp.data.orientVel;
    shadowTree.orientTime = tmp.data.orientTime;
    shadowTree.mesh = tmp.data.mesh;
    shadowTree.scale = tmp.data.scale;
    shadowTree.isCleared = tmp.data.isCleared;
    shadowTree.contextId = tmp.data.contextId;
    shadowTree.isConnected = tmp.data.isConnected;

    interHandler(tmp.data,shadowTree, 'connectedCallback', markedObjects,nameService);
    shadowTree.isSuspended = tmp.data.isSuspended;
    shadowTree.suspendedVelocity = tmp.data.suspendedVelocity;
    shadowTree.suspendedOrientationVelocity = tmp.data.suspendedOrientationVelocity;
    interHandler(tmp.data,shadowTree, 'proxAddedCallback', markedObjects,nameService);
    interHandler(tmp.data,shadowTree, 'proxRemovedCallback', markedObjects,nameService);
}


/**
 Called to tree-ify a timer object
 */
function processTimer(timer,shadowTree,markedObjects,nameService)
{
    throw 'Error with processTimer since change';
    
    if (! std.persist.checkTimer(timer))
        throw 'Error processing timer.  First arg passed in must be a timer.';

    throw 'Error.  Will need to change how values are pushed to shadowTree for timers.';
    
    shadowTree[std.persist.TYPE_FIELD_STRING] = std.persist.TIMER_OBJECT_STRING;

    //loading presence data into a non-restorable so that
    //won't try walking subsequent data when restoring.
    var tmp  = new std.persist.NonRestorable();
    tmp.data = timer.getAllData();
    shadowTree.isCleared = tmp.data.isCleared;
    shadowTree.contextId = tmp.data.contextId;
    shadowTree.period = shadowTree.isCleared ? null : tmp.data.period;
    shadowTree.timeRemaining = shadowTree.isCleared ? null : tmp.data.timeRemaining;
    shadowTree.isSuspended = shadowTree.isCleared ? null : tmp.data.isSuspended;

    if (shadowTree.isCleared)
        shadowTree.callback = null;
    else
        interHandler(tmp.data,shadowTree,'callback',markedObjects,nameService);
    
}



/**
 Called to tree-ify a visible object
 */
function processVisible(vis, shadowTree,markedObjects,nameService)
{
    throw 'Error with processVisible since change';
    
    if (! std.persist.checkVisible(vis))
        throw 'Error processing visible.  First argument passed in must be a visible.';

    throw 'Error.  Will need to change how values are pushed to shadowTree for visibles.';
    
    shadowTree[std.persist.TYPE_FIELD_STRING] = std.persist.VISIBLE_OBJECT_STRING;

    //loading presence data into a non-restorable so that
    //won't try walking subsequent data when restoring.
    var tmp = new std.persist.NonRestorable();
    tmp.data = pres.getAllData();

    for (var s in tmp.data)
        shadowTree[s] = tmp.data[s];
}


