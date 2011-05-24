var NO_RESTORE_STRING = '__noRestore';


//each object is labeled with an id.  Can point to that id for loops
//this is how to index into the object to get its id
var ID_FIELD_STRING   = '__hidden_id';

//each object in the serialized graph has a type.  This is the field for that type in
//the shadow graph.  
var TYPE_FIELD_STRING = '__hidden_type';

//for loops
var POINTER_OBJECT_STRING = '__pointer_object';
var POINTER_FIELD_STRING  = '__pointer_field';


/* Type constants */
var GET_TYPE_STRING      = '__getType';

var SYSTEM_TYPE_STRING   =    'system';
var PRESENCE_TYPE_STRING =  'presence';
var VISIBLE_TYPE_STRIG   =   'visible';
var TIMER_TYPE_STRIG     =     'timer';
var PRESENCE_ENTRY_TYPE_STRING = "presenceEntry";


function NonRestorable()
{
    this[NO_RESTORE_STRING] = true;
}

function checkNonRestorable(obj)
{
    if (typeof(obj)   == 'object')
        return (NO_RESTORE_STRING in obj);

    return false;
}



//builds a parallel tree of to restore objects.
//__markField is set to true for each object that we find on the way.
//

function checkpoint(presenceEntries)
{
    var globalObj = (function()
                  {
                      return this;
                  })();

    var idGenerator     = new NonRestorable();
    idGenerator.id      = 0;
    
    var shadowTree      = new NonRestorable();
    var markedObjectMap = new NonRestorable();

    markAndBranch(globalObj, shadowTree, idGenerator, markedObjectMap);
    var serialized = system.serialize(shadowTree);
    system.print(serialized);
    system.prettyprint(shadowTree);

    system.__debugFileWrite(serialized, 'testFile.em.bu');
    
}


/**
 @param value/object to check if it has been marked.
 @return returns true if value passed in is an object that has been marked.  Returns null otherwise
 */
function checkMarked(objToCheck, markedObjects)
{
    return (objToCheck in markedObjects) ? true : null ;
}


/**
 @param value.  if it's an object (and not null), then it marks the object
 @exception Throws an exception if the object that we're supposed to mark has already been marked.
 @return returns the id that the object has been marked with.
 */
function mark(objToMark, markedObjects, idGenerator)
{
    if (checkMarked(objToMark,markedObjects))
        throw 'Error.  Asking to mark object that\'s already been marked';

    if (typeof(objToMark) != 'object')
        throw 'Error.  Asking to mark a non-object: '+objToMark.toString();            


    
    markedObjects[objToMark] = idGenerator.id;
    ++idGenerator.id;
    return markedObjects[objToMark];
}


/**
 @param objGraphCatalog This is the root of the object graph that we have not yet copied to shadowTree
 @param shadowTree: This is the serialized version of the object graph.  We progressively add to it.
 @param idGenerator: For each object that we grab, we increment the id field in the idGenerator field and tag that object with a unique id in the id field.
 @param markedObjects is a map of objects that we have already cataloged.  Index of map is the object.  Points to the id of that object.
 */
function markAndBranch(objGraphCatalog, shadowTree, idGenerator, markedObjects)
{
    if (checkNonRestorable(objGraphCatalog))
        return;

    //set a type for this object;
    if (checkSystem(objGraphCatalog))
    {
        processSystem(objGraphCatalog,shadowTree,idGenerator,markedObjects);
        return;
    }
    if (checkPresence(objGraphCatalog))
    {
        processPresence(objGraphCatalog,shadowTree,idGenerator,markedObjects);
        return;            
    }
    if (checkTimer(objGraphCatalog))
    {
        processTimer(objGraphCatalog,shadowTree,idGenerator,markedObjects);
        return;
    }
    if (checkVisible(objGraphCatalog))
    {
        processVisible(objGraphCatalog,shadowTree,idGenerator,markedObjects);
        return;
    }
    if (checkPresenceEntry(objGraphCatalog))
    {
        processPresenceEntry(objGraphCatalog,shadowTree,idGenerator,markedObjects);
        return;
    }
    
    for (var s in objGraphCatalog)
    {
        if (typeof(objGraphCatalog[s]) != 'object')
            shadowTree[s] = objGraphCatalog[s];
        else
            interHandler(objGraphCatalog,shadowTree,s,idGenerator,markedObjects);
    }
}


function interHandler(objGraphCatalog,shadowTree,field,idGenerator,markedObjects)
{
    //check if got null object.
    if (objGraphCatalog[field] == null)
    {
        shadowTree[field] = null;
        return;
    }
    
    if (checkNonRestorable(objGraphCatalog))
        return;

    
    var theMark = checkMarked(objGraphCatalog[field],markedObjects);
    if (theMark == null)
    {
        //may want to do special things here to determine if it is a special object;
        //means that we have not already tagged this object
        shadowTree[field] = new NonRestorable();
        shadowTree[field][ID_FIELD_STRING] =  mark(objGraphCatalog[field],markedObjects,idGenerator);
        markAndBranch(objGraphCatalog[field],shadowTree[field],idGenerator,markedObjects);
    }
    else
    {
        //is marked.  create pointer for it.
        shadowTree[field] = new NonRestorable();
        shadowTree[field][TYPE_FIELD_STRING] = POINTER_OBJECT_STRING;
        shadowTree[field][POINTER_FIELD_STRING] = theMark;
    }
}




/**
 @param obj to check if it is a system object
 @return true if system object.  false otherwise
 */
function checkSystem(obj)
{
    if (GET_TYPE_STRING in obj)
        return (obj[GET_TYPE_STRING]() == SYSTEM_TYPE_STRING);

    return false;
}

/**
 @param obj to check if it is a presence object
 @return true if presence object.  false otherwise
 */
function checkPresence(obj)
{
    if (GET_TYPE_STRING in obj)
        return (obj[GET_TYPE_STRING]() == PRESENCE_TYPE_STRING);

    return false;
}

/**
 @param obj to check if it is a visible object
 @return true if visible object.  false otherwise
 */
function checkVisible(obj)
{
    if (GET_TYPE_STRING in obj)
        return (obj[GET_TYPE_STRING]() == VISIBLE_TYPE_STRING);

    return false;
}

/**
 @param obj to check if it is a timer object
 @return true if timer object.  false otherwise
 */
function checkTimer(obj)
{
    if (GET_TYPE_STRING in obj)
        return (obj[GET_TYPE_STRING]() == TIMER_TYPE_STRING);

    return false;
}

/**
 @param obj to check if it is a PresenceEntry object
 @return true if PresenceEntry object.  false otherwise
 */
function checkPresenceEntry(obj)
{
    if (GET_TYPE_STRING in obj)
        return (obj[GET_TYPE_STRING]() == PRESENCE_ENTRY_TYPE_STRING);

    return false;
}





/**
 Called to tree-ify system object.
 @param objGraphCatalog This is a system object that has not already been added to markedObjects.  Must copy an entry for it into the shadow tree.  Know it is not a non-restorable.
 @param shadowTree: This is the serialized version of the object graph.  We progressively add to it.
 @param idGenerator: For each object that we grab, we increment the id field in the idGenerator field and tag that object with a unique id in the id field.
 @param markedObjects is a map of objects that we have already cataloged.  Index of map is the object.  Points to the id of that object.
 */
function processSystem(objGraphCatalog, shadowTree, idGenerator, markedObjects)
{
    if (!checkSystem(objGraphCatalog))
        throw 'Error in process system.  First argument must be a system object.';
    
    //set a type for shadowTree (it already has an id)
    shadowTree[TYPE_FIELD_STRING] = SYSTEM_TYPE_STRING;
    
    var sysData = new NonRestorable();
    sysData.selfMap = objGraphCatalog.getAllData();
    
    shadowTree.selfMap = new NonRestorable();
    var theMark = checkMarked(sysData.selfMap,markedObjects);
    if (theMark == null)
    {
        //not marked
        shadowTree.selfMap[ID_FIELD_STRING] =  mark(sysData.selfMap, markedObjects,idGenerator);
        shadowTree.selfMap[TYPE_FIELD_STRING] = SELF_MAP_OBJECT_STRING;
        
        //go down each branch
        processSelfMap(sysData.selfMap,shadowTree.selfMap,idGenerator,markedObjects);
    }
    else
    {
        //is marked.  create pointer for it.
        shadowTree.selfMap = new NonRestorable();
        shadowTree.selfMap[TYPE_FIELD_STRING] = POINTER_OBJECT_STRING;
        shadowTree.selfMap[POINTER_FIELD_STRING] = theMark;
    }
}

/**
 Called to tree-ify selfMap.
 @param sMap should be the selfMap pointed at by system.
 @see processSystem
 */
function processSelfMap(sMap,shadowTree,idGenerator,markedObjects)
{
    for (s in sMap)
    {
        if (s == system.__NULL_TOKEN__)
            continue;

        interHandler(sMap,shadowTree,s,idGenerator,markedObjects);
    }
}

/**
 Called to tree-ify a presence entry object.
 @param pEntry should be a PresenceEntry.  If it is not, throws exception.
 @see processSystem
 */
function  processPresenceEntry(pEntry,shadowTree,idGenerator, markedObjects)
{
    if (checkPresenceEntry(pEntry))
        throw 'Error processing presence entry.  First arg passed in must be a presence entry';
    
    
    shadowTree[TYPE_FIELD_STRING] = PRESENCE_ENTRY_OBJECT_STRING;
    
    shadowTree.sporef  = pEntry.sporef;

    interHandler(pEntry, shadowTree,'presObj',idGenerator,markedObjects);
    interHandler(pEntry, shadowTree,'proxResultSet',idGenerator,markedObjects);
    interHandler(pEntry, shadowTree,'proxAddCB',idGenerator,markedObjects);
    interHandler(pEntry, shadowTree,'proxRemCB',idGenerator,markedObjects);
}


/**
 Called to tree-ify a presence object.
  @see processSystem
 */
function processPresence(pres,shadowTree,idGenerator,markedObjects)
{
    if (checkPresence(pres))
        throw 'Error processing presence.  First arg passed in  must be a presence.';
    
    
    shadowTree[TYPE_FIELD_STRING] = PRESENCE_OBJECT_STRING;

    //loading presence data into a non-restorable so that
    //won't try walking subsequent data when restoring.
    var tmp = new NonRestorable();
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

    interHandler(tmp.data,shadowTree, 'connectedCallback', idGenerator, markedObjects);
    shadowTree.isSuspended = tmp.data.isSuspended;
    shadowTree.suspendedVelocity = tmp.data.suspendedVelocity;
    shadowTree.suspendedOrientationVelocity = tmp.data.suspendedOrientationVelocity;
    interHandler(tmp.data,shadowTree, 'proxAddedCallback', idGenerator, markedObjects);
    interHandler(tmp.data,shadowTree, 'proxRemovedCallback', idGenerator, markedObjects);
}


/**
 Called to tree-ify a timer object
 */
function processTimer(timer,shadowTree,idGenerator,markedObjects)
{
    if (checkTimer(timer))
        throw 'Error processing timer.  First arg passed in must be a timer.';

    shadowTree[TYPE_FIELD_STRING] = TIMER_OBJECT_STRING;

    //loading presence data into a non-restorable so that
    //won't try walking subsequent data when restoring.
    var tmp  = new NonRestorable();
    tmp.data = timer.getAllData();
    shadowTree.isCleared = tmp.data.isCleared;
    shadowTree.contextId = tmp.data.contextId;
    shadowTree.period = shadowTree.isCleared ? null : tmp.data.period;
    shadowTree.timeRemaining = shadowTree.isCleared ? null : tmp.data.timeRemaining;
    shadowTree.isSuspended = shadowTree.isCleared ? null : tmp.data.isSuspended;

    if (shadowTree.isCleared)
        shadowTree.callback = null;
    else
        interHandler(tmp.data,shadowTree,'callback',idGenerator,markedObjects);
    
}



/**
 Called to tree-ify a visible object
 */
function processVisible(vis, shadowTree,idGenerator,markedObjects)
{
    if (checkVisible(vis))
        throw 'Error processing visible.  First argument passed in must be a visible.';

    shadowTree[TYPE_FIELD_STRING] = VISIBLE_OBJECT_STRING;

    //loading presence data into a non-restorable so that
    //won't try walking subsequent data when restoring.
    var tmp = new NonRestorable();
    tmp.data = pres.getAllData();

    for (var s in tmp.data)
        shadowTree[s] = tmp.data[s];
}


