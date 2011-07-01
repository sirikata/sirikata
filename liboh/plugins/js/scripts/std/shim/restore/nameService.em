if (typeof(std.persist) === 'undefined')
    throw new Error('Error.  Do not import this file directly.  Only import it from persistService.em');




/*Create a separate namespace for name service;*/
std.persist.NameService = function()
{
    /**
     Keys are object references; values are unique integers
     ("names").
     */
    // var objectsToNames  =  {  };
     
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


     /**
      Does Not Exist.  
      Returned when maps calling lookup on do not have a record for
      the index searching for.
      */
     this.DNE = -99;

     /**
      Prints all objects that we're responsible for and their names 
      */
     this.__debugPrint = function ()
     {
         system.print('\nPrinting in name service\'s debug print\n');
         for (var s in namesToObjects)
         {
             system.print('\n');
             system.print(s);
             system.print(namesToObjects[s]);
         }
         system.print('\nDone\n');
     };

     /**
      @param name to lookup.  (name will be an integer starting at
      zero).
      @returns if object exists, returns the object.  If it does not,
      returns DNE
      */
     this.lookupObject = function(name)
     {
         if (name in namesToObjects)
             return namesToObjects[name];

         return this.DNE;
     };

     /**
      @param obj to lookup name of.  (name will be an integer starting
      at zero).
      @returns if object exists, returns the object.  If it does not,
      returns DNE
      */
     this.lookupName = function (obj)
     {
         for (var s in namesToObjects)
         {
             if (obj == namesToObjects[s])
                 return s;
         }

         return this.DNE;
     };


     /**
      @param objIndex object reference to index into hierarchyMap
      @param markedMap contains a map indexed on object references
      that point to all the names of objects that are marked
      @throw Throws an exception if the object that we're entering a
      markedMap for does not have an entry in objectsToNames.
      */
      this.enterSubtreeObjects = function (objIndex, markedMap)
      {
          if (this.lookupName(objIndex) == this.DNE)
              throw new Error('Error.  Not tracking this object in nameService when calling enterMarkedMap');

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
     this.lookupSubtreeObjects = function(objIndex)
     {
         throw new Error('Error in lookupSubtreeObjects');
          if (objIndex in hierarchyMap)
              return hierarchyMap[objIndex];

          return this.DNE;
      };


      /**
       @param objToInsert into name service.
       @return Returns the name for the object.
       */
      this.insertObject = function (objToInsert)
      {
          var ind = this.lookupName(objToInsert);
          if (ind != this.DNE)
          {
              system.print('\nAlready had inserted object\n');
              return ind;                  
          }

          namesToObjects[whichNameOn] = objToInsert;
          return whichNameOn++;
      };


    /**
     @param objToInsert into name service
     @param nameToUse How to index into the new name in the service.

     Note.  If nameToUse is greater than or equal to which name to
     insert into next, we increase the value for which name to insert
     into next to avoid possible collisions.
     */
     this.insertObjectWithName = function (objToInsert,nameToUse)
     {
         namesToObjects[nameToUse]    = objToInsert;

         if (nameToUse >= whichNameOn)
             whichNameOn = nameToUse + 1;
     };
    
     
 };
