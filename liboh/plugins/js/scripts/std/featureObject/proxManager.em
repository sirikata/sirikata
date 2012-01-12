

////////////Now prox manager///////////

(function()
 {
     //map
     //keys: sporef of presence
     //values: array of actual visible objects
     var pResultSet = { };

     // //map
     // //keys: sporef of visible awaiting data on
     // //value: Tuparray of visible objects awaiting data, 
     // var awaitingFeatureData ={ };
     
     //map
     //keys: sporef of presence
     //values: array of proximity added functions
     var pAddCB = { };

     //map
     //keys: sporef of presence
     //values: array of proximity removed functions
     var pRemCB = {};

     //map
     //keys: sporef of presence
     //values: presences themselves.
     var sporefToPresences = {};

     system.onPresenceConnected(
         function(newPres)
         {
             sporefToPresences[newPres.toString()] = newPres;
             pResultSet[newPres.toString()]        = {};
             pAddCB[newPres.toString()]            = {};
             pRemCB[newPres.toString()]            = {};
         });

     
     
     function ProxManager()
     {
     }


     ProxManager.prototype.getProxResultSet = function(pres)
     {
         if (!(pres.toString() in pResultSet))
             throw new Error('Error getting prox result set.  ' +
                             'No record of presence.');

         return pResultSet[pres.toString()];
     };
     
     ProxManager.prototype.setProxAddCB = function(pres,proxAddCB)
     {
         if (!(pres.toString() in pAddCB))
         {
             throw new Error('Cannot add prox callback because do ' +
                             'not have associated presence in ProxManager.');
         }

         
         var pArray = pAddCB[pres.toString()];
         
	 for (var i = 0; i <= pArray.length; i++)
	 {
	     if ((typeof(pArray[i]) == 'undefined') || (pArray[i] == null))
	     {
		 pArray[i] = proxAddCB;
		 return i;
	     }
	 }
         //shouldn't ever get here (notice the <= on exit condition of
         //for loop).  Just added so that syntax
         //highlighting of emacs doesn't keep bothering me.
         return null;
     };

     ProxManager.prototype.setProxRemCB = function (pres,proxRemCB)
     {
         if (!(pres.toString() in pRemCB))
         {
             throw new Error('Cannot add prox rem callback because do ' +
                             'not have associated presence in ProxManager.');
         }
         
         var pArray = pRemCB[pres.toString()];
         
	 for (var i = 0; i <= pArray.length; i++)
	 {
	     if ((typeof(pArray[i]) == 'undefined') || (pArray[i] == null))
	     {
		 pArray[i] = proxRemCB;
		 return i;
	     }
	 }
         //shouldn't ever get here (notice the <= on exit condition of
         //for loop).  Just added so that syntax
         //highlighting of emacs doesn't keep bothering me.
         return null;
     };


     ProxManager.prototype.delProxAddCB = function(pres,addID)
     {
         if (!(pres.toString() in pAddCB))
         {
             throw new Error('Cannot del prox add callback because do ' +
                             'not have associated presence in ProxManager.');
         }

         var pArray = pAddCB[pres.toString()];
         
         
         if (!(typeof(pArray[addID]) == 'undefined'))
             pArray[addID] = null;
     };

     ProxManager.prototype.delProxRemCB = function(pres,remID)
     {
         if (!(pres.toString() in pRemCB))
         {
             throw new Error('Cannot del prox rem callback because do ' +
                             'not have associated presence in ProxManager.');
         }

         var pArray = pRemCB[pres.toString()];
         
         
         if (!(typeof(pArray[addID]) == 'undefined'))
             pArray[addID] = null;         
     };

     
     ProxManager.prototype.proxAddedEvent = function (pres,visibleObj)
     {
         if (!(pres.toString() in pAddCB))
         {
             throw new Error('Cannot call add event callback because do ' +
                             'not have associated presence in ProxManager.');
         }

         pResultSet[pres.toString()][visibleObj.toString()] = visibleObj;
         var pArray = pAddCB[pres.toString()];
         //trigger all non-null non-undefined callbacks
         system.changeSelf(pres);
	 for (var i in pArray)
         {
	     if ((typeof(pArray[i]) != 'undefined') && (pArray[i] != null))
		 pArray[i](visibleObj);                 
         }
     };

     
     ProxManager.prototype.proxRemovedEvent = function (pres,visibleObj)
     {
         if (!(pres.toString() in pRemCB))
         {
             throw new Error('Cannot call rem event callback because do ' +
                             'not have associated presence in ProxManager.');
         }

         var pArray = pRemCB[pres.toString()];
         
         //remove from to proxResultSet
        delete pArray[visibleObj.toString()];
        system.changeSelf(pres);
        //trigger all non-null non-undefined callbacks
	for (var i in pArray)
	    if ((typeof(pArray[i]) != 'undefined') && (pArray[i] != null))
		pArray[i](visibleObj);
     };


     system.__registerProxManager(new ProxManager());
     
 }
)();

