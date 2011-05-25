if (typeof(std.persist) === 'undefined')
    throw 'Error.  Do not import this file directly.  Only import it from persistService.em';



/*Create a separate namespace for name service;*/
(function()
 {

     /**
      Keys are object references; values are unique integers
      ("names").
      */
     var objectsToNames  =  {  };
     
     /**
      Keys are integers ("names") each unique to a single object.
      Values are object references
      */
     var namesToObjects  = {  };

     /**
      Keys are object references.  Values are single objects that
      contain the references and names for every object that is
      reachable from that object.  Not all objects that are stored in
      objectsToNames and namesToObjects will have entries in
      hierarchyMap.  Just those that were the roots for a request to
      persist.
      */
     var hierarchyMap    = {  };

     
     var whichNameOn = 0;
     
     nameService = {};

     /**
      Does Not Exist.  
      Returned when maps calling lookup on do not have a record for
      the index searching for.
      */
     nameService.DNE = -1;

     /**
      @param name to lookup.  (name will be an integer starting at
      zero).
      @returns if object exists, returns the object.  If it does not,
      returns DNE
      */
     nameService.lookupObject = function(name)
     {
         if (name in namesToObjects)
             return namesToObjects[name];

         return nameService.DNE;
     };
     
     /**
      @param obj to lookup name of.  (name will be an integer starting
      at zero).
      @returns if object exists, returns the object.  If it does not,
      returns DNE
      */
     nameService.lookupName = function (obj)
     {
         if (obj in objectsToNames)
             return objectsToNames[obj];

         return nameService.DNE;
     };

     
     /**
      @param objIndex object reference to index into hierarchyMap
      @param markedMap contains a map indexed on object references
      that point to all the names of objects that are marked
      @throw Throws an exception if the object that we're entering a
      markedMap for does not have an entry in objectsToNames.
      */
     nameService.enterSubtreeObjects = function (objIndex, markedMap)
     {
         if (nameService.lookupName(objIndex) == nameService.DNE)
             throw 'Error.  Not tracking this object in nameService when calling enterMarkedMap';
         
         hierarchyMap[objIndex] = markedMap; //marked map includes current object
     };


     /**
      Checks hierarchyMap to see if objIndex has an entry in it that
      contains all the objects that are in its subtree.  If it does,
      returns subtree.  If it does not, returns DNE.
      
      @param objIndex object refernce to index into hierarchyMap
      @returns DNE if have no record for objIndex in hierarchyMap.
      Returns a map containing all the object references (as keys) and
      names (as values) of the objects reachable from this object.
      */
     nameService.lookupSubtreeObjects = function(objIndex)
     {
         if (objIndex in hierarchyMap)
             return hierarchyMap[objIndex];

         return nameService.DNE;
     };
     
     
     /**
      @param objToInsert into name service.
      @return Returns the name for the object.
      */
     nameService.insertObject = function (objToInsert)
     {
         var ind = nameService.lookupName(objToInsert);
         if (ind != nameService.DNE)
             return ind;

         objectsToNames[objToInsert] = whichNameOn;
         namesToObjects[whichNameOn] = objToInsert;

         return whichNameOn++;
     };
     
 })();
