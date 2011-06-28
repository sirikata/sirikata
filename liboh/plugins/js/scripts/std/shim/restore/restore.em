if (typeof(std.persist) === 'undefined')
    throw new Error('Error.  Do not import this file directly.  Only import it from persistService.em');


(function()
 {

     /**
      Each element of this array contains the unique id given to the
      presence object when it was being persisted.  And each element
      correpsonds to a presence that is trying to be restored in the
      world.
      */
     var allPresStillRest = [];

     var mRestoring = false;
     
     function registerPresenceStillRestoring(presObjID)
     {
         allPresStillRest.push(presObjID);
     }

     
     /**
      Each restored presence has an onConnected callback.  The
      onConnected callback should be generated from this afterRestored
      function.  All that the callback does is update the nameService
      that we're using with the new presence object and removes this
      presence from the list of presences that we are restoring that
      are still waiting to be connected (in allPresStillRest).

      Second stage of restoration (fixing up pointers to other
      objects) waits until all presences have been connected (polls
      allPresStillRest to see if have zero length).

      @param {int} presObjID, unique name that was given to presence
      before restore.  (ie name that goes into nameService.)

      @param nameService nameService used during restoration.
      */
     function afterRestored(presObjID,nameService)
     {
         var returner = function()
         {
             var toRemove = [];
             for (var s in allPresStillRest)
             {
                 //keeps track of all indices to remove from
                 //allPresStillRest
                 toRemove.push(s);
             }

             //insert the presence into nameService;
             nameService.insertObjectWithName(system.self,presObjID);

             
             toRemove.reverse();
             for (var s in toRemove)
                 delete allPresStillRest[ toRemove[s]  ];
         };
         return returner;
     }


     function checkIsTriplet(toCheck)
     {
         if ((typeof(toCheck) !== 'object') || (!('length' in toCheck)) || (toCheck.length != 3))
             return false;

         return true;
     } 
     
     function tripletGetIndexName(triplet)
     {
         if (!checkIsTriplet(triplet))
         {
             system.prettyprint(triplet);
             throw new Error('Error in tripletGetIndexName.  Requires triplet to be passed in');
         }

         return triplet[0];
     }

     function tripletGetValue(triplet)
     {
         if (!checkIsTriplet(triplet))
             throw new Error('Error in tripletGetValue.  Requires triplet to be passed in');

         var type = tripletGetType(triplet);
         if (type == 'number')
             return parseFloat(triplet[1]);
         if (type == 'string')
             return triplet[1];
         if (type == 'boolean')
             return (triplet[1] == 'true');
         
         
         return triplet[1];         
     }

     function tripletGetType(triplet)
     {
         if (!checkIsTriplet(triplet))
             throw new Error('Error in tripletGetType.  Requires triplet to be passed in');
         return triplet[2];         
     }
     
     
    /**
     @param {array} triplet Array containing three values.  Indices of
     array follow:
       0 : index
       1 : value to set to/object pointer/'null'
       2 : type record

     @return {boolean} Returns true if the value in this object is a
     value type
     */
    function tripletIsValueType (triplet)
    {
        if (! checkIsTriplet(triplet))
            throw new Error('Error in checkTripletIsValueType.  Requires triplet to be passed in');

        var tripType = tripletGetType(triplet);

        if ((tripType != 'object') && (tripType != 'function') &&
            (tripType!=std.persist.SYSTEM_TYPE_STRING) &&
            (tripType!=std.persist.PRESENCE_OBJECT_TYPE_STRING) &&
            (tripType!=std.persist.VISIBLE_TYPE_STRING) &&
            (tripType!=std.persist.TIMER_TYPE_STRING) &&
            (tripType!=std.persist.PRESENCE_ENTRY_TYPE_STRING) &&
            (tripType!=std.persist.FUNCTION_OBJECT_TYPE_STRING) &&
            (tripType!=std.persist.BASIC_OBJECT_TYPE_STRING))
            return true;

        return false;
    };

     /**
      @param {int} ptrId The unique name of the object that should be
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
     };

     /**
      locaCopyToPoint[index] should end up pointing to the object represented
      by the lookup into nameService of ptrId

      @see registerFixupObjectPointer
      @see fixSinglePtr
      */
     function fixSinglePtr (ptrId, index,localCopyToPoint,nameService)
     {
         if ((typeof(localCopyToPoint) != 'object') || (localCopyToPoint== null))
             throw new Error('Error in fixSinglePtr.  Should not have received non-obect or null ptr record.');

         var fixedObj = nameService.lookupObject(ptrId);
         if (fixedObj == nameService.DNE)
             throw new Error('Error in fixSinglePtr.  Have no record of object that you are trying to point to.');

         localCopyToPoint[index] = fixedObj;
     };

     

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
     };




     /**
      fixReferences opens the keyName table, and returns an object whose
      subgraph is rooted at the object with id ptrId. Note: all objects the
      root object references may not be linked to correctly.  Need to call
      performPtrFinalFixups on ptrsToFix after running this function.

      @param {keyName}, Passed to the backend.  This is the entry to
      access.

      @param {int}, ptrId, Each object has a unique id.  ptrId is the
      identifier for the object that we should be fixing references for (in
      the keyName table).

      @param {array} ptrsToFix is an array that tracks all objects that we
      will need to fixup the references too.  See performFixupPtr function.

      @param nameService can be used to lookup the ids for objects that
      we've already re-read.

      @param cb callback to invoke when complete. Passed success flag (boolean)
      and, if successful, an object whose subgraph is rooted at the object with id ptrId.

      */
     function fixReferences(keyName, ptrId, ptrsToFix, nameService, cb)
     {
         readObject(
             keyName,ptrId,
             std.core.bind(finishFixReferences, undefined, keyName, ptrId, ptrsToFix, nameService, cb) // + success, object
         );
     }


     function finishFixReferences(keyName, ptrId, ptrsToFix, nameService, cb, success, unfixedObj) {
         if (!success)
         {
             cb(false);
             return;
         }

         var id = unfixedObj['mID'];
         if (id != ptrId) //throw 'Error: ptrId and object id must be identical';
             cb(false);

         if (nameService.lookupObject(ptrId) != nameService.DNE) // throw "Error.  Called fixReferences on an object I've already visited";
             cb(false);

         var type = unfixedObj['type'];
         if (type == std.persist.FUNCTION_OBJECT_TYPE_STRING)
             restoreFunction(keyName,unfixedObj,ptrId,ptrsToFix,nameService,cb);
         else if (type == std.persist.PRESENCE_OBJECT_TYPE_STRING)
             restorePresence(keyName,unfixedObj,ptrId,ptrsToFix,nameService,cb);
         else if (type == std.persist.BASIC_OBJECT_TYPE_STRING)
             restoreBasicObject(keyName,unfixedObj,ptrId,ptrsToFix,nameService,cb);
         else if (type == std.persist.VEC_TYPE_STRING)
             restoreVec3Object(keyName,unfixedObj,ptrId,ptrsToFix,nameService,cb);
         else if (type == std.persist.QUAT_TYPE_STRING)
             restoreQuatObject(keyName,unfixedObj,ptrId,ptrsToFix,nameService,cb);
         else if (type == std.persist.VISIBLE_TYPE_STRING)
             restoreVisibleObject(keyName,unfixedObj,ptrId,ptrsToFix,nameService,cb);
         else
             throw new Error('Error in fixReferences.  Do not have any other types in the system to restore from.');
     }

     /**
      @see fixReferences for arguments
      Returns a new visible object.
      */
     function restoreVisibleObject(keyName, visRecord,ptrId,ptrsToFix,nameService,cb)
     {
         var pToFix = [];
         restoreBasicObject(
             keyName,visRecord,ptrId,pToFix,nameService,
             std.core.bind(finishRestoreVisibleObject, undefined, keyname, visRecord, ptrId, pToFix, nameService, cb)
         );
     };
     function finishRestoreVisibleObject(keyName, visRecord,ptrId,pToFix,nameService,cb, success, toRestoreFrom) {
         if(!success) {
             cb(false);
             return;
         }

         //perform the fixups for the objects that this presence was
         //trying to point to.  (Eg. pos object, vel obj, etc.)
         performPtrFinalFixups(pToFix,nameService);

         var returner = system.createVisible(
             toRestoreFrom.sporef,
             toRestoreFrom.pos,
             toRestoreFrom.vel,
             toRestoreFrom.posTime,
             toRestoreFrom.orient,
             toRestoreFrom.orientVel,
             toRestoreFrom.orientTime,
             toRestoreFrom.pos,
             toRestoreFrom.scale,
             toRestoreFrom.mesh,
             toRestoreFrom.physics,
             toRestoreFrom.solidAngleQuery
         );

         cb(true, returner);
     }
     
     /**
      @see fixReferencs for arguments.
      Returns a new vec3.
      */
     function restoreVec3Object(keyName, unfixedObj,ptrId,ptrsToFix,nameService, cb)
     {
         //lkjs: may want some check here.
         var x = getValueField(unfixedObj,'x');
         var y = getValueField(unfixedObj,'y');
         var z = getValueField(unfixedObj,'z');
         var returner = new util.Vec3(x,y,z);
         nameService.insertObjectWithName(returner,ptrId);
         cb(true, returner);
     }

     /**
      @see fixReferencs for arguments.
      Returns a new quaternion.
      */
     function restoreQuatObject(keyName, unfixedObj,ptrId,ptrsToFix,nameService, cb)
     {
         var x = getValueField(unfixedObj,'x');
         var y = getValueField(unfixedObj,'y');
         var z = getValueField(unfixedObj,'z');
         var w = getValueField(unfixedObj,'w');

         var returner = new util.Quaternion(x,y,z,w);
         nameService.insertObjectWithName(returner,ptrId);
         cb(true, returner);
     }
     

     /**
      @param unfixedObj Is an object that is read in directly from the
      backend.  It has three types of fields: 1) triplets, 2) mID (obj
      id), 3) type (obj type).
      
      @param Field name that we're looking for in one of the triplets.

      @return returns the value of the triplet associated with
      filedName

      @throw Throws an error if fieldName does not exist as a field in
      any of unfixedObj's triplets.
      
      This function runs through all the triplets in unfixedObj, and
      returns the value of the triplet that matches the field.
      */
     function getValueField(unfixedObj,fieldName)
     {
         for (var s in unfixedObj)
         {
             if ((s == 'mID') || (s == 'type'))
                 continue;

             var propValTypeTriple = unfixedObj[s];
             
             var tripFieldName = tripletGetIndexName(unfixedObj[s]);
             if (tripFieldName == fieldName)
                 return tripletGetValue(propValTypeTriple);
         }
         throw new Error('Error in getValueField.  Have no field named: '+ fieldName);
     }
     
     /**
      For keyName,ptrId,ptrsToFix, and nameService, @see fixReferences.

      unfixedPres has all the data that you would get from a call to
      getAllData on a presence.
      
      */
     function restorePresence(keyName,unfixedPres,ptrId,ptrsToFix,nameService, cb)
     {
         var pToFix = [];
         restoreBasicObject(
             keyName,unfixedPres,ptrId,pToFix,nameService,
             std.core.bind(finishRestorePresence, undefined, keyName, unfixedPres, ptrId, pToFix, nameService, cb) // + success, obj
         );
     }
     function finishRestorePresence(keyName,unfixedPres,ptrId,pToFix,nameService, cb, success, toRestoreFrom) {
         if(!success) {
             cb(false);
             return;
         }
         
         //perform the fixups for the objects that this presence was
         //trying to point to.  (Eg. pos object, vel obj, etc.)
         performPtrFinalFixups(pToFix,nameService);

         for (var s in system.presences)
         {
             if (system.presences[s].toString() == toRestoreFrom.sporef)
             {
                 cb(true,system.presences[s]);
                 return;
             }
         }

         system.print('\n\nDEBUG: Got into presence restore.\n');
         var onConnectCB = afterRestored(ptrId,nameService);         
         system.restorePresence(
             toRestoreFrom.sporef,
             toRestoreFrom.pos,
             toRestoreFrom.vel,
             toRestoreFrom.posTime,
             toRestoreFrom.orient,
             toRestoreFrom.orientVel,
             toRestoreFrom.orientTime,
             toRestoreFrom.mesh,
             toRestoreFrom.scale,
             toRestoreFrom.isCleared,
             toRestoreFrom.contextId,
             toRestoreFrom.isConnected,
             onConnectCB,
             toRestoreFrom.isSuspended,
             toRestoreFrom.suspendedVelocity,
             toRestoreFrom.suspendedOrientationVelocity,
             toRestoreFrom.solidAngleQuery
         );
         
         //tells the system that we have begun trying to restore this
         //presence, and not to continue with later stages of
         //restoration (fixing looped object pointers) until the
         //system has connected this presence.
         registerPresenceStillRestoring(ptrId);
         cb(true, null); // Not sure about this being null....
     };

     
     /**
      For keyName,ptrId,ptrsToFix, and nameService, @see fixReferences.

      unfixedFunc right now just has one field: 'funcField' with the
      text of the function that we're creating.
      */
     function restoreFunction(keyName,unfixedFunc,ptrId,ptrsToFix,nameService, cb)
     {
         //FIXME: lkjs;  GROSS!
         var funcTriple = unfixedFunc[0];

         var index = tripletGetIndexName(funcTriple);
         if (index != 'funcField')
             throw new Error('Error restoring function.  Restoring object does not have funcField.');

         var funcAsString = tripletGetValue(funcTriple);

         var returner = null;
         var success = true;
         try
         {
             returner = system.eval(funcAsString);
         }
         catch (excep)
         {
             system.print('ERROR: Error in restoreFunction.  Trying to restore a function with faulty syntax.');
             returner = function(){};
             success = false;
         }
         nameService.insertObjectWithName(returner,ptrId);
         cb(success, returner);
     }


     /**
      For keyName, ptrId,ptrsToFix, and nameService, @see fixReferences.

      @param unfixedObj is the object that we get back from reading
      from reading from backend.  It is a basic object (not a
      function, presence, system obj, etc.).  This function runs
      through all of its fields and registers them to be fixed up.
      
      */
     function restoreBasicObject(keyName,unfixedObj,ptrId,ptrsToFix,nameService,cb)
     {
         var returner = { };
         nameService.insertObjectWithName(returner,ptrId);
         //run through all the fields in unfixedObj.  If the field is a
         //value type, point returner to the new field right away.  If the
         //field is an object type and we have a copy of that object, then
         //point returner to that new object right away.  If the field is
         //an object type and we do not have a copy of that object, then
         //register returner to be fixed-up with the new pointer later.
         
         var unfinished_fields = 0;
         var did_callback = false;
         var finish_field_cb = function(success, val) {
             if (!success && !did_callback) {
                 did_callback = true;
                 cb(false);
                 return;
             }
             unfinished_fields -= 1;
             if (unfinished_fields == 0) {
                 did_callback = true;
                 cb(true, returner);
             }
         };
         for (var s in unfixedObj)
         {
             if ((s == 'mID') || (s == 'type'))
                 continue;

             var index = tripletGetIndexName(unfixedObj[s]);

             //sets to value type
             if (tripletIsValueType(unfixedObj[s]))
                 returner[index] = tripletGetValue(unfixedObj[s]);
             else
             {
                 if ((tripletGetType(unfixedObj[s])) == 'null')
                     returner[index] = null;
                 else
                 {
                     var ptrIdInner  = tripletGetValue(unfixedObj[s]);
                     var ptrObj = nameService.lookupObject(ptrIdInner);
                     if (ptrObj != nameService.DNE)
                     {
                         //already have the object.  just link to it.
                         returner[index] = ptrObj;
                     }
                     else
                     {
                         //will have to register this pointer to be fixed up
                         registerFixupObjectPointer( ptrIdInner ,index,returner,ptrsToFix);
                         unfinished_fields += 1;
                         fixReferences(keyName,ptrIdInner,ptrsToFix,nameService, std.core.bind(finish_field_cb, undefined));
                     }
                 }
             }
         }

         // Special case for empty objects
         if (unfinished_fields == 0)
             cb(true, returner);
     };


     /**
      Returns true if we're in the middle of a restore operation.
      Otherwise, returns false.
      */
     std.persist.inRestore = function()
     {
         return mRestoring;
     };


     /**
      @see std.persist.restoreFromAndGetNamesAsync
      */
     std.persist.restoreFromAsync = function (keyName,cb)
     {
         return std.persist.restoreFromAndGetNamesAsync(keyName,0,cb);
     };
     

     
     /**
      This function should be used if the object graph that you are
      trying to restore has presences in it, and you need to wait for
      the system to actually connect these presences.

      @param cb Takes in three parameters, first is restored obj graph
      (if pres failure, all presences become null objs.), second
      boolean for success (ie true if eveything restored
      appropriately, and space allowed new presences to reconnect,
      false otherwise), the third is the nameService that we used.
      */
     std.persist.restoreFromAndGetNamesAsync = function (keyName,id,cb)
     {
         if (std.persist.inRestore())
             throw new Error('Error, cannot request additional restores when in middle of current restore.  Check back later.');
         mRestoring = true;
         
         var nameService = new std.persist.NameService();
         var ptrsToFix = [];
         fixReferences(
             keyName, id,ptrsToFix,nameService,
             std.core.bind(finishRestoreFromAndGetNamesAsync, undefined, keyName, id, cb, ptrsToFix, nameService) // + success, obj
         );
     };

     
     var finishRestoreFromAndGetNamesAsync = function (keyName,id,cb, ptrsToFix, nameService, success, returner) {
         if (!success) {
             mRestoring = false;
             cb(false);
             return;
         }
         if (allPresStillRest.length == 0)
         {
             performPtrFinalFixups(ptrsToFix,nameService);
             mRestoring = false;
             cb(true,returner,nameService);
             return;
         }

         //if we haven't restored all presences after five seconds, then we're just not going to.         
         var restoreCheckback = function()
         {
             // means that we were trying to restore a presence;
             // should just return that presence instead;
             if (returner == null)
             {
                 var tRet = nameService.lookupObject(id);
                 if (tRet != nameService.DNE)
                 {
                     returner = tRet;
                     system.changeSelf(returner.toString());
                 }
             }
             performPtrFinalFixups(ptrsToFix,nameService);
             mRestoring = false;
             allPresStillRest = [];
             if (allPresStillRest.length == 0)
                 cb(true, returner, nameService);
             else
                 cb(false, returner, nameService);
         };
         
         system.timeout(5,restoreCheckback);
     };
     
     
 })();
