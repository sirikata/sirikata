if (typeof(std.persist) === 'undefined')
    throw 'Error.  Do not import this file directly.  Only import it from persistService.em';



/**
 Reads in a filename as a string.  Calls deserialize on that string to
 get back an object.  Runs through that object graph, and uses
 nameService to restore it.

 @param {String} name of file to read a serialized object graph in
 from.

 @return {Object} Returns a copy of the object that had been put into persistent storage 
 */
function restoreFrom(filename,id)
{
    return restoreFromAndGetNames(filename,id)[0];
}


/**
  @param {String} name of file to read a serialized object graph in
 from.

 @return {Array} Returns an array.  First index is the copy of the object that had been put into persistent storage.  Second index is a name service that you can use to name and identify objects in the restored subgraph.
 */
function restoreFromAndGetNames(keyName,id)
{
    var nameService = new std.persist.NameService();
    var ptrsToFix = [];
    var returner = fixReferences(keyName, id,ptrsToFix,nameService);
    performPtrFinalFixups(ptrsToFix,nameService);
    return [returner,nameService];
}


function tripletIsValueType(triplet)
{
    if ((typeof(triplet) !== 'object') || (!('length' in triplet)) || (triplet.length != 3))
    {
        system.print('\nAbout to exception\n');
        system.prettyprint(triplet);
        throw 'Error in checkTripletIsValueType.  Requires triplet to be passed in';            
    }


    if (typeof(triplet[2]) !== 'object')
        return true;

    return false;
}


/**
 fixReferences opens the keyName table, and returns an object whose
 subgraph is rooted at the object with id ptrId. Note: all objects the
 root object references may not be linked to correctly.  Need to call
 performPtrFinalFixups on ptrsToFix after running this function.
 
 @param {keyName}, Passed to the backend.  This is the table to
 access.
 
 @param {int}, ptrId, Each object has a unique id.  ptrId is the
 identifier for the object that we should be fixing references for (in
 the keyName table).

 @param {array} ptrsToFix is an array that tracks all objects that we
 will need to fixup the references too.  See performFixupPtr function.

 @param nameService can be used to lookup the ids for objects that
 we've already re-read.

 @return {object} Returns an object whose subgraph is rooted at the
 object with id ptrId. 

 
 */
function fixReferences(keyName, ptrId,ptrsToFix,nameService)
{
    var returner = { };

    var unfixedObj = std.persist.Backend.read(keyName,ptrId);

    var id = unfixedObj['mID'];
    if (nameService.lookupObject(id) != nameService.DNE)
        throw "Error.  Called fixReferences on an object I've already visited";

    nameService.insertObjectWithName(returner,id);
    

    //run through all the fields in unfixedObj.  If the field is a
    //value type, point returner to the new field right away.  If the
    //field is an object type and we have a copy of that object, then
    //point returner to that new object right away.  If the field is
    //an object type and we do not have a copy of that object, then
    //register returner to be fixed-up with the new pointer later.
    for (var s in unfixedObj)
    {
        if (s == 'mID')
            continue;
        
        var index = unfixedObj[s][0];
        
        if (tripletIsValueType(unfixedObj[s]))
            returner[index] = unfixedObj[s][1];
        else
        {

            if ((unfixedObj[s][2]) == 'null')
                returner[index] = null;
            else
            {
                var ptrIdInner  = unfixedObj[s][1];
                var ptrObj = nameService.lookupObject(ptrId);
                if (ptrObj != nameService.DNE)
                {
                    //already have the object.  just link to it.
                    returner[index] = ptrObj;
                }
                else
                {
                    //will have to register this pointer to be fixed up
                    registerFixupObjectPointer( ptrIdInner ,index,returner,ptrsToFix);
                    fixReferences(keyName,ptrIdInner,ptrsToFix,nameService);
                }
            }
        }
    }
    return returner;
}


/**
 @param {int} ptrId.  The unique name of the object that should be
 referenced by calling localCopyToPoint[index]
 @param index of the local object to perform fixup on.
 @param {object} localCopyToPoint localCopyToPoint[index] should be the new
 object pointer
 @param {array} allToFix an array.  Push the above three arguments onto that
 array (as an array).

 After rest of deserialization occurs, final step is to call
 performFinalFixups with this array as an argument.
 performPtrFinalFixups traverses array, and points
 localCopyToPoint[index] to the objects described by objPtrToFix.
 */
function registerFixupObjectPointer(ptrId, index, localCopyToPoint, allToFix)
{
    allToFix.push([ptrId, index,localCopyToPoint]);
}


/**
 @param {array} allToFix array.  Each element has three values. [0]
 ptrId, [1] index, [2] localCopyToPoint.  

 Runs through array generated by calls to registerFixupObjectPointer,
 and individually performs fixups for each one.

 @see registerFixupObjectPointer
 @see fixSinglePtr
 */
function performPtrFinalFixups(allToFix,nameService)
{
    for (var s in allToFix)
        fixSinglePtr(allToFix[s][0],allToFix[s][1],allToFix[s][2],nameService);
}

/**
 locaCopyToPoint[index] should end up pointing to the object represented
 by the lookup into nameService of ptrId

 @see registerFixupObjectPointer
 @see fixSinglePtr
 */
function fixSinglePtr(ptrId, index,localCopyToPoint,nameService)
{
    if ((typeof(localCopyToPoint) != 'object') || (localCopyToPoint== null))
        throw 'Error in fixSinglePtr.  Should not have received non-obect or null ptr record.';

    var fixedObj = nameService.lookupObject(ptrId);
    if (fixedObj == nameService.DNE)
        throw 'Error in fixSinglePtr.  Have no record of object that you are trying to point to.';

    localCopyToPoint[index] = fixedObj;
}

