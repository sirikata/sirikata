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
function checkpointPartialPersist(objToPersistFrom, filename)
{
    if ((objToPersistFrom == null) || (typeof(objToPersistFrom) != 'object'))
        throw 'Error.  Can only checkpoint objects.';
    
    var shadowTree      = new std.persist.NonRestorable();

    //after call to markAndBranch, markedObjectMap will contain all
    //objects that were reachable from the root object
    //objToPersistFrom.
    var markedObjectMap = new std.persist.NonRestorable();

    //add myself to markedObjectMap before beginning.
    var rootID = mark(objToPersistFrom, markedObjectMap);
    shadowTree[std.persist.ID_FIELD_STRING] = rootID;

    //recursively traverse objet graph.
    markAndBranch(objToPersistFrom, shadowTree,markedObjectMap);

    //log the subtree of objects reachable from objToPersistFrom in
    //nameService
    nameService.enterSubtreeObjects(objToPersistFrom, markedObjectMap);

    var serialized = system.serialize(shadowTree);

    // system.print('\n\nDEBUG\n');
    // system.prettyprint(shadowTree);
    // system.print('\n\n---\n');

    // var deser = system.deserialize(serialized);
    // system.print('\n\nDEBUG2\n');
    // system.prettyprint(deser);
    // system.print('\n\n---2\n');
    
    
    //save the file out to disk.
    var fName = typeof(filename) == 'undefined' ? 'testFile.em.bu' : filename;
    system.__debugFileWrite(serialized, fName);
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
function mark(objToMark, markedObjects)
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
 @param shadowTree: This is the serialized version of the object graph.  We progressively add to it.
 @param markedObjects is a map of objects that we have already cataloged.  Keys are object references.  Values are unique names associated with each object given by nameService.
 */
function markAndBranch(objGraphCatalog, shadowTree, markedObjects)
{
    if (std.persist.checkNonRestorable(objGraphCatalog))
        return;

    //set a type for this object;
    if (std.persist.checkSystem(objGraphCatalog))
    {
        processSystem(objGraphCatalog,shadowTree,markedObjects);
        return;
    }
    if (std.persist.checkPresence(objGraphCatalog))
    {
        processPresence(objGraphCatalog,shadowTree,markedObjects);
        return;            
    }
    if (std.persist.checkTimer(objGraphCatalog))
    {
        processTimer(objGraphCatalog,shadowTree,markedObjects);
        return;
    }
    if (std.persist.checkVisible(objGraphCatalog))
    {
        processVisible(objGraphCatalog,shadowTree,markedObjects);
        return;
    }
    if (std.persist.checkPresenceEntry(objGraphCatalog))
    {
        processPresenceEntry(objGraphCatalog,shadowTree,markedObjects);
        return;
    }
    if (std.persist.checkFunctionObject(objGraphCatalog))
    {
        processFunction(objGraphCatalog,shadowTree,markedObjects);
        return;
    }

    
    for (var s in objGraphCatalog)
    {
        // check typeof s;
        // lkjs;
        if (typeof(objGraphCatalog[s]) != 'object')
            pushValueType(objGraphCatalog,s,shadowTree);
        else
            interHandler(objGraphCatalog,shadowTree,s,markedObjects);
    }
}


/**
 @param {object} objGraphCatalog the local copy of the object whose field we're trying to persist.
 @param index: the name of the field of the local copy of the object we're trying to persist.
 @param {object} shadowTree: the serialized tree of the local object that we're trying to copy.
 */
function pushValueType(objGraphCatalog,index,shadowTree)
{
    std.persist.nonRestorePush(shadowTree,std.persist.wrapPropValPair(index,objGraphCatalog[index]));
}

/**

 */
function interHandler(objGraphCatalog,shadowTree,field,markedObjects)
{
    //check if got null object.
    if (objGraphCatalog[field] == null)
    {
        std.persist.nonRestorePush(shadowTree,std.persist.wrapPropValPair(field,null));
        return;
    }
    
    if (std.persist.checkNonRestorable(objGraphCatalog))
        return;
    
    var theMark = checkMarked(objGraphCatalog[field],markedObjects);
    var valToPush = new std.persist.NonRestorable();
    if (theMark == null)
    {
        //may want to do special things here to determine if it is a special object;
        //means that we have not already tagged this object
        valToPush[std.persist.ID_FIELD_STRING]       =  mark(objGraphCatalog[field],markedObjects);
        var index = std.persist.nonRestorePush(shadowTree,std.persist.wrapPropValPair(field,valToPush));
        // markAndBranch(objGraphCatalog[field],valToPush,markedObjects);
        markAndBranch(objGraphCatalog[field],shadowTree[index],markedObjects);

        //clean up lkjs;
        // shadowTree[field] = new std.persist.NonRestorable();
        // shadowTree[field][std.persist.ID_FIELD_STRING] =  mark(objGraphCatalog[field],markedObjects);
        // markAndBranch(objGraphCatalog[field],shadowTree[field],markedObjects);
    }
    else
    {
        //is marked.  create pointer for it.
        valToPush[std.persist.TYPE_FIELD_STRING]     = std.persist.POINTER_OBJECT_TYPE_STRING;
        valToPush[std.persist.POINTER_FIELD_STRING]  = theMark;
        std.persist.nonRestorePush(shadowTree,std.persist.wrapPropValPair(field, valToPush));

        
        // shadowTree[field] = new std.persist.NonRestorable();
        // shadowTree[field][std.persist.TYPE_FIELD_STRING]     = std.persist.POINTER_OBJECT_TYPE_STRING;
        // shadowTree[field][std.persist.POINTER_FIELD_STRING]  = theMark;
    }
}





/**
 Called to tree-ify system object.
 @param objGraphCatalog This is a system object that has not already been added to markedObjects.  Must copy an entry for it into the shadow tree.  Know it is not a non-restorable.
 @param shadowTree: This is the serialized version of the object graph.  We progressively add to it.
 @param markedObjects is a map of objects that we have already cataloged.  Index of map is the object.  Points to the id of that object.
 */
function processSystem(objGraphCatalog, shadowTree, markedObjects)
{
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
        processSelfMap(sysData.selfMap,shadowTree.selfMap,markedObjects);
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
function processSelfMap(sMap,shadowTree,markedObjects)
{
    for (s in sMap)
    {
        if (s == system.__NULL_TOKEN__)
            continue;

        interHandler(sMap,shadowTree,s,markedObjects);
    }
}

/**
 Called to tree-ify a presence entry object.
 @param pEntry should be a PresenceEntry.  If it is not, throws exception.
 @see processSystem
 */
function  processPresenceEntry(pEntry,shadowTree, markedObjects)
{
    if (std.persist.checkPresenceEntry(pEntry))
        throw 'Error processing presence entry.  First arg passed in must be a presence entry';

    throw 'Error.  Will need to change how values are pushed to shadowTree for presenceEntries.';
    
    shadowTree[std.persist.TYPE_FIELD_STRING] = std.persist.PRESENCE_ENTRY_OBJECT_STRING;
    
    shadowTree.sporef  = pEntry.sporef;

    interHandler(pEntry, shadowTree,'presObj',markedObjects);
    interHandler(pEntry, shadowTree,'proxResultSet',markedObjects);
    interHandler(pEntry, shadowTree,'proxAddCB',markedObjects);
    interHandler(pEntry, shadowTree,'proxRemCB',markedObjects);
}



/**
 Called to tree-ify a function object.  Write now, not capturing the closure, just calling toString on function.  Ugh.
 @see processSystem
 */
function processFunction(func,shadowTree,markedObjects)
{
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
function processPresence(pres,shadowTree,markedObjects)
{
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

    interHandler(tmp.data,shadowTree, 'connectedCallback', markedObjects);
    shadowTree.isSuspended = tmp.data.isSuspended;
    shadowTree.suspendedVelocity = tmp.data.suspendedVelocity;
    shadowTree.suspendedOrientationVelocity = tmp.data.suspendedOrientationVelocity;
    interHandler(tmp.data,shadowTree, 'proxAddedCallback', markedObjects);
    interHandler(tmp.data,shadowTree, 'proxRemovedCallback', markedObjects);
}


/**
 Called to tree-ify a timer object
 */
function processTimer(timer,shadowTree,markedObjects)
{
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
        interHandler(tmp.data,shadowTree,'callback',markedObjects);
    
}



/**
 Called to tree-ify a visible object
 */
function processVisible(vis, shadowTree,markedObjects)
{
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


