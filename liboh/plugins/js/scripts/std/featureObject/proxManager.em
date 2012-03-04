system.require('featureObject.em');

(function()
 {
     //if we request the feature object this many times from another
     //visible, and do not receive it, then just fire the prox event
     //for the visible setting featureObject to be undefined.
     var NUM_TRIES_TO_GET_FEATURE_OBJECT = 3;
     //if we haven't heard back from visible in this amount of time,
     //then either re-issue the feature object request (up to
     //NUM_TRIES_TO_GET_FATURE_OBJECT times), or just pass through
     //undefined for the visible's featureObject (if tried that many
     //times, and didn't get a response).
     var TIME_TO_WAIT_FOR_FEATURE_OBJECT_RESP = 10;


     //Want to avoid firing prox callbacks in cases where we get a
     //removal quickly followed by an addition.  (can happen if a
     //dynamic object switches to static tree, even if object does not
     //move at all).  To mask this, we wait this number of seconds before
     //actually firing removal notifications.
     var TIME_TO_WAIT_BETWEEN_REMOVAL_AND_NOTIFICATION = 1;
     
     //map
     //keys: sporef of presence
     //values: map of actual visible objects
     var pResultSet = {};

     /**
      These get placed into the awaitingFeatureData and
      haveFeatureDataFor map
      @param {visible} vis - The visible that we request features for.
      @param {presence} featGrabber - The presence that we're using to
      request features and listen for feature updates.
      @param {presence} subscriber (optional) - A presence
      that wants to receive updates on vis.
      */
     function SubscriptionElement (vis,featGrabber,proxMan,subscriber)
     {
         this.vis = vis;
         this.featGrabber =featGrabber;
         this.proxMan = proxMan;
         this.subscribers = {};
         if (typeof(subscriber) != 'undefined')
             this.subscribers[subscriber.toString()] = subscriber;

         this.awaitingTimeout  = {};
     }

     /**
      @return {bool} -- True if should fire a prox callback after this
      event, false otherwise.

      Note: the reason for the return value here is that we try to
      mask removals followed by quick additions (reason: the interface
      that pinto provides us doesn't actually tell us when something
      has moved out of our proximity query, it tells us when something
      has moved out of a tree that we were watching).  For instance,
      if a visible moves from the dynamic tree to the static tree, we
      get a removal and addition, even if the visible itself has not
      moved at all.
      */
     SubscriptionElement.prototype.addSubscriber =
         function(presToAdd)
     {
         var pString = presToAdd.toString();
         this.subscribers[pString] = presToAdd;

         //may have gotten a quick removal followed by addition.  if
         //did, then clear waiting timeout.
         if (pString in this.awaitingTimeout)
         {
             this.awaitingTimeout[pString].clear();
             delete this.awaitingTimeout[pString];
             return false;
         }

         return true;
     };
     

     //returns true if there are no subscribers left.
     SubscriptionElement.prototype.noSubscribers =
         function()
     {
         for (var s in this.subscribers)
             return false;

         return true;
     };
     
     SubscriptionElement.prototype.startRemoveSubscriber =
         function(presToRemove)
     {
         var timer = system.timeout(
             TIME_TO_WAIT_BETWEEN_REMOVAL_AND_NOTIFICATION,
             std.core.bind(elemFinalizeRemove,undefined,this,presToRemove));

         //if already had a timer waiting on removing the subscriber,
         //cancel it first.
         var pString = presToRemove.toString();
         if (pString in this.awaitingTimeout)
             this.awaitingTimeout[pString].clear();
         
         this.awaitingTimeout[pString] = timer;
     };

     function elemFinalizeRemove(subElement,presToRemove)
     {
         if (presToRemove.toString() in subElement.subscribers)
             delete subElement.subscribers[presToRemove.toString()];

         finalizeRemove(
             presToRemove,subElement.vis,subElement.proxMan,subElement);
     }


     
     //map
     //keys: sporef of visible awaiting data on
     //value: SubscriptionElement
     var awaitingFeatureData = {};

     //map
     //keys: sporef of visibles
     //keys: SubscriptionElement
     var haveFeatureDataFor  = {};
     
     //map
     //keys: sporef of presence
     //values: array of proximity added functions.  note, some
     //elements may be null if the proximity added function was
     //deregistered.
     var pAddCB = {};

     //map
     //keys: sporef of presence
     //values: array of proximity removed functions.  note, some
     //elements may be null if the proximity removed function was
     //deregistered.
     var pRemCB = {};

     //map
     //keys: sporef of presence
     //values: presences themselves.
     var sporefToPresences = {};

     system.onPresenceConnected(
         function(newPres)
         {
             var nString = newPres.toString();
             sporefToPresences[nString] = newPres;
             pResultSet[nString]        = {};
             pAddCB[nString]            = [];
             pRemCB[nString]            = [];
         });

     
     //Does not hold internal data.  Instead, has private-like
     //variables defined above.
     function ProxManager()
     {
     }


     ProxManager.prototype.getProxResultSet = function(pres)
     {
         var pString = pres.toString();
         if (!(pString in pResultSet))
             throw new Error('Error getting prox result set.  ' +
                             'No record of presence.');

         return pResultSet[pString];
     };
     
     ProxManager.prototype.setProxAddCB = function(pres,proxAddCB)
     {
         var pString = pres.toString();
         if (!(pString in pAddCB))
         {
             throw new Error('Cannot add prox callback because do ' +
                             'Not have associated presence in ProxManager.');
         }

         
         var pArray = pAddCB[pString];
         
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
         var pString = pres.toString();
         if (!(pString in pRemCB))
         {
             throw new Error('Cannot add prox rem callback because do ' +
                             'not have associated presence in ProxManager.');
         }

         var pArray = pRemCB[pString];
         
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
         var pString = pres.toString();
         if (!(pString in pAddCB))
         {
             throw new Error('Cannot del prox add callback because do ' +
                             'not have associated presence in ProxManager.');
         }

         var pArray = pAddCB[pString];
         
         if (!(typeof(pArray[addID]) == 'undefined'))
             pArray[addID] = null;
     };

     ProxManager.prototype.delProxRemCB = function(pres,remID)
     {
         var pString = pres.toString();
         
         if (!(pString in pRemCB))
         {
             throw new Error('Cannot del prox rem callback because do ' +
                             'not have associated presence in ProxManager.');
         }

         var pArray = pRemCB[pString];
         
         
         if (!(typeof(pArray[addID]) == 'undefined'))
             pArray[addID] = null;         
     };

     
     /**
      Gets called by system whenever visibleObj is within proximity
      range of pres.  Will trigger proxadded callbacks to be called
      if we already have the featureData for visibleObj.  Otherwise,
      queue visibleObj onto awaitingFeatureData.
      */
     ProxManager.prototype.proxAddedEvent = function (pres,visibleObj)
     {
         var pString = pres.toString();
         var vString = visibleObj.toString();
         //ignore events where you have entered your own prox set.
         if (pString == vString)
             return;
         
         if (!(pString in pAddCB))
         {
             throw new Error('Cannot call add event callback because do ' +
                             'not have associated presence in ProxManager.');
         }

         if (vString in haveFeatureDataFor)
         {
             //we already have feature data for this visible.  add pres
             //as subscriber to haveFeatureDataFor map and trigger callback.
             var subElement = haveFeatureDataFor[vString];
             var issueCallback = subElement.addSubscriber(pres);
             if (issueCallback)
                 this.triggerAddCallback(pres,subElement.vis);
         }
         else
         {
             //short-circuits: does not send message to get feature data.
             var subElement =
                 new SubscriptionElement(visibleObj,pres,this,pres);
             
             haveFeatureDataFor[vString] = subElement;
             this.triggerAddCallback(pres,subElement.vis);
         }
     };
     

     /**
      Gets called by system whenever visibleObj is within proximity
      range of pres.  Will trigger proxadded callbacks to be called
      if we already have the featureData for visibleObj.  Otherwise,
      queue visibleObj onto awaitingFeatureData.
      */
     // ProxManager.prototype.proxAddedEvent = function (pres,visibleObj)
     // {
     //     //ignore events where you have entered your own prox set.
     //     if (pres.toString() == visibleObj.toString())
     //         return;
         
     //     if (!(pres.toString() in pAddCB))
     //     {
     //         throw new Error('Cannot call add event callback because do ' +
     //                         'not have associated presence in ProxManager.');
     //     }


         
     //     if (visibleObj.toString() in haveFeatureDataFor)
     //     {
     //         //we already have feature data for this visible.  add pres
     //         //as subscriber to haveFeatureDataFor map and trigger callback.
     //         var subElement = haveFeatureDataFor[visibleObj.toString()];
     //         subElement.addSubscriber(pres);
     //         this.triggerAddCallback(pres,subElement.vis);
     //     }
     //     else
     //     {
     //         //we do not already have feature data for this visible.
     //         //Check if we're waiting on feature data for this object.
     //         //If we are, then, just add pres as a subscriber.  If we
     //         //are not, initiate feature object subscription using
     //         //pres as the requester.  If we are, waiting, then add
     //         //pres as a subscriber for this feature data.
     //         if (visibleObj.toString() in awaitingFeatureData)
     //         {
     //             var subElement = awaitingFeatureData[visibleObj.toString()];
     //             subElement.addSubscriber(pres);
     //         }
     //         else
     //         {
     //             var subElement = new SubscriptionElement(visibleObj,pres,pres);
     //             this.beginFeatureSubscribe(pres, visibleObj);
     //             awaitingFeatureData[visibleObj.toString()] = subElement;
     //         }
     //     }
     // };

     
     //tells pres to listen for feature vector from visibleObj.
     //numTriesLeft should be left blank except when this function
     //calls itself.  (the idea is that it tries to get feature object
     //data from a presence multiple times and numTriesLeft is a
     //simple way for the function to keep track of how many times
     //we've attempted to get feature object data from other side.
     ProxManager.prototype.beginFeatureSubscribe =
         function(pres,visibleObj,numTriesLeft)
     {
         if (typeof(numTriesLeft) == 'undefined')
             numTriesLeft = NUM_TRIES_TO_GET_FEATURE_OBJECT;

         //If we try multiple times, and still don't get a response, the visible
         //probably is not running the feature object protocol.  We
         //can't do anything about that, and instead fire callback
         //with undefined passed through for featureObject.
         if (numTriesLeft <=0)
         {
             this.subscribeComplete(pres,visibleObj,undefined);
             return;
         }
         --numTriesLeft;

         
         //see handleFeautreObjectSubscribe (featureObject.em) to see
         //what format the feature object subscription request message
         //should take.
         pres # {'presFeatureSubRequest':true} >> visibleObj >>
             [std.core.bind(handleFeatureData,undefined,this,pres,visibleObj),
              TIME_TO_WAIT_FOR_FEATURE_OBJECT_RESP,
              std.core.bind(
                  this.beginFeatureSubscribe,
                  this,pres,visibleObj,numTriesLeft)];
         
         // //for now, simple sanity check.  After 1 second, release,
         // //saying that featureData is 1.
         // system.timeout(1,std.core.bind(this.subscribeComplete,this,pres,visibleObj,1));
     };

     function handleFeatureData(proxMan,mPres,vis,featureMsg,visSender)
     {
         //see handleFeatureObjectSubscribe for structure of reply
         //mesages.
         proxMan.subscribeComplete(mPres,vis,featureMsg.data);
     }
     

     //Call this function when you want pres to unsubscribe for
     //feature data from visibleObj.
     ProxManager.prototype.killSubscription = function(pres,visibleObj)
     {
         //short-circuits.
         return;
         
         //see handleFeautreObjectRequestUnsubscribe message handler
         //in featureObject.em to see what the format of a
         //killSubscription message should look like.
         pres # {'presFeatureReqUnsub': true} >> visibleObj >> [];
     };


     
     
     //should get called when a presence has received feature data
     //from visibleObj.
     ProxManager.prototype.subscribeComplete =
         function(pres,visibleObj,featureData)
     {
         var vString = visibleObj.toString();
         
         visibleObj.featureObject= featureData;
         if (!(vString in awaitingFeatureData))
         {
             throw new Error('Error in subscribeComplete.  No longer '+
                             'have visibleObj in awaitingFeatureData.');                 
         }

         var subElement = awaitingFeatureData[vString];
         delete awaitingFeatureData[vString];

         
         if (subElement.noSubscribers())
         {
             //no one is waiting for/actually cares about the new
             //data.
             this.killSubscription(pres,visibleObj);
             return;
         }

         //know that we've received the data and that others want to
         //hear about it.  add subElement to haveFeatureDataFor map
         //and notify all subscribers that visibleObj (with
         //featureData) is in its prox result set and add visibleObj
         //to result set.
         haveFeatureDataFor[vString] = subElement;
         for (var s in subElement.subscribers)
         {
             this.triggerAddCallback(subElement.subscribers[s],
                                     subElement.vis);
         }
     };

     //actually fire callback for pres's having seen visibleObj in-world
     //and add visibleObj to result set.
     ProxManager.prototype.triggerAddCallback = function(pres,visibleObj)
     {
         var pString = pres.toString();
         pResultSet[pString][visibleObj.toString()] = visibleObj;
         var pArray = pAddCB[pString];
         //trigger all non-null non-undefined callbacks
         system.changeSelf(pres);
	 for (var i in pArray)
         {
	     if ((typeof(pArray[i]) != 'undefined') && (pArray[i] != null))
		 pArray[i](visibleObj);                 
         }
     };



     //called by system when pinto actually states that visibleObj is
     //distant from pres.  If the visible exists in awaitingFeatureData
     //then we just remove a subscriber from it.  If the visible exists in
     //haveFeatureDataFor, then fire removed callback, decrement
     //subscribers.  If no subscribers left in haveFeatureDataFor, then
     //unsubscribe from feature data, and remove element from
     //haveFeatureDataFor.
     ProxManager.prototype.proxRemovedEvent = function (pres,visibleObj)
     {
         var pString = pres.toString();
         var vString = visibleObj.toString();
         if (pString == vString)
             return;
         
         if (vString in awaitingFeatureData)
             awaitingFeatureData[vString].startRemoveSubscriber(pres);                 
         else
         {
             //this line ensures that visibleObj will maintain
             //featureData associated with stored vis.
             visibleObj = haveFeatureDataFor[vString].vis;
             var subElem = haveFeatureDataFor[vString];
             subElem.startRemoveSubscriber(pres);
         }
     };



     //actually call remove callback
     function finalizeRemove (pres,visibleObj,proxMan,subElem)
     {
         if (subElem.noSubscribers())
         {
             proxMan.killSubscription(pres,visibleObj);
             delete haveFeatureDataFor[visibleObj.toString()];
         }

         //actually fire removed callback.
         proxMan.triggerRemoveCallback(pres,visibleObj);
     };



     ProxManager.prototype.triggerRemoveCallback = function(pres,visibleObj)
     {
         var pString = pres.toString();
         
         if (!(pString in pRemCB))
         {
             throw new Error('Cannot call rem event callback because do ' +
                             'not have associated presence in ProxManager.');
         }
         if (!(pString in pResultSet))
         {
             throw new Error('Cannot call rem event callback for presence ' +
                             'because no presence in pResultSet matches.');
         }

         //remove from to proxResultSet
         delete pResultSet[pString][visibleObj.toString()];

         //actually issue callbacks
         var pArray = pRemCB[pString];
         system.changeSelf(pres);
         //trigger all non-null non-undefined callbacks

	 for (var i in pArray)
         {
	     if ((typeof(pArray[i]) != 'undefined') && (pArray[i] != null))
		 pArray[i](visibleObj);                          
         }

     };


     //This function handles all the feature object updates.  For now,
     //we have a very dumb protocol.  We transmit the full feature
     //object instead of the parts that have changed/been updated.
     function handleFeatureObjUpdates(updateMsg,sender)
     {
         var update = updateMsg.featureObjUpdate;
         var sString = sender.toString();
         
         if (!(sString in haveFeatureDataFor))
         {
             throw new Error('Error upon reception of feature object ' +
                             'update.  Had no subscription to sender ' +
                             'of message.');
         }

         var localVis = haveFeatureDataFor[sString].vis;
         localVis.featureObject = update;

         //sends ack to other side so update sender knows we're still
         //alive.
         updateMsg.makeReply({}) >> [];
     }
     //see sendFeaturesToSubscribers function in featureObject.em for
     //format of featureObject update messages.
     handleFeatureObjUpdates << {'featureObjUpdate'::};

     
     //actually register an instance of proxmanager with system.
     system.__registerProxManager(new ProxManager());
 }
)();