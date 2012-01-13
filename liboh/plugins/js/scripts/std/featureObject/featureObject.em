/**
 Note: this file should only be imported once.  It maintains a map of
 presences hosted by this entity.  Associated with each presence are
 two things:

   1: A list of other presence's in the world that want to know more
   about this presence (as indicated by their having sent a
   subscription message); and

   2: A feature vector (really an object with fields and values) that
   contains information that the user wants to push to all listening presences.

will use transaction model to write changes to object.
 
 */

std.featureObject = {};

system.require('featureChangeTransaction.em');



(function()
{
    var presencesToFeatureObjectMap = {};
    var TIME_TO_ACK_UPDATE = 10;
    
    /**
     Whenever we create a new presence, just add an entry for it
     into feature object map.
     */
    function ourPresenceConnected(newPres)
    {
        presencesToFeatureObjectMap[newPres.toString()]  =
            {
                data: {},
                dataCommitMap: {},
                subscribers: {},
                updateCount: 0
            };
    }
    system.onPresenceConnected(ourPresenceConnected);

    /*
     Note: may want to add a conditional subscribe.;
     Subscribe me if I am like this;
     */
    
    /**
     Whenever we get a subscription message, reply with our feature
     data (and add subscriber for future events) or tell the other
     side that query failed.
     */
    function handleFeatureObjectSubscribe(msg,subscriber)
    {
        var selfIndex = system.self.toString();
        if (selfIndex in presencesToFeatureObjectMap)
        {
            var replyObj = {
                success: true,
                data: presencesToFeatureObjectMap[selfIndex].data
                };

            msg.makeReply(replyObj) >> [];
            
            presencesToFeatureObjectMap[selfIndex].subscribers[subscriber.toString()] = subscriber;
        }
        else
        {
            var replyObj = {
                success: false,
                data: udefined
            };
            msg.makeReply(replyObj) >> [];
        }
    }
    //see ProxManager.beginFeatureSubscribe (in proxManager.em) to see
    //what format the feature object subscription request message
    //should take.
    handleFeatureObjectSubscribe << {'presFeatureSubRequest'::};



    /**
     Whenever we get an unsubscription message, remove the presence
     from our subscribers, and notify them that they have been
     unsubscribed of their own volition.  Do not need to ack request.
     */
    function handleFeatureObjectRequestUnsubscribe(msg,subscriber)
    {
        if (!(system.self.toString() in presencesToFeatureObjectMap))
        {
            throw new Error('Error when received a request to unsubscribe.  ' +
                            'No record of receiver in featureObjectMap.');
        }

        var featureObjer = presencesToFeatureObjectMap[system.self.toString()];

        //actually remove the subscriber from map.
        if (subscriber.toString() in featureObjer.subscribers)
            delete featureObjer.subscribers[subscriber.toString()];                
    }
    
    //see ProxManager.killSubscription for what message is sent when
    //requesting unsubscription.  (In proxManager.em.)
    handleFeatureObjectRequestUnsubscribe << {'presFeatureReqUnsub' ::};

    

    /**
     If we drop an update message without acking it, then after some
     time, we'll get unsubscribed.  If we had just missed that
     message, and then logged back in, this unsubscription would be
     incorrect, and we'd get into a poor state.  As a result, if we
     receive an unsubscription notice, we just resubscribe.
     */
    function resubscribeAfterUnsubscribe(msg,sender)
    {
        var subscriptionMsg = {
          'featureObjectSubscribeRequest' : true
        };

        subscriptionMsg >> sender >> [];
    }
    resubscribeAfterUnsubscribe <<{'featureObjectUnsubscribe'::};
        
    
    
    std.featureObject.addField = function(index,data,pres)
    {
        singleCommit(
            std.featureObject.Action.ADD,index,data,pres);
    };

    std.featureObject.removeField = function(index,pres)
    {
        singleCommit(
            std.featureObject.Action.ADD,index,undefined,pres);
    };
    
    std.featureObject.changeField = function(index,data,pres)
    {
        singleCommit(
            std.featureObject.Action.CHANGE,index,data,pres);
    };

    function singleCommit(type, index, data,pres)
    {
        if (typeof(pres) == 'undefined')
        {
            if (typeof(system.self) == 'undefined')
            {
                throw new Error ('Error changing feaure field: ' +
                                 'unknown presence.');                    
            }
            pres = system.self;                
        }

        var committer = std.featureObject.beginFeatureTransaction(pres);
        if (type == std.featureObject.Action.ADD)
            committer.addField(index,data);
        else if (type == std.featureObject.Action.REMOVE)
            committer.removeField(index);
        else if (type == std.featureObject.Action.CHANGE)
            committer.changeField(index,data);
        else
        {
            throw new Error('Unrecognized commit type in singleCommit ' +
                            'of featureObject.');                
        }


        committer.commit();
    };
    
    /**
     @param {presence} presToChange (optional)
     */
    std.featureObject.beginFeatureTransaction = function(presToChange)
    {
        if (typeof(presToChange) == 'undefined')
        {
            if (tyepeof(system.self) == 'undefined')
            {
                throw new Error ('Error in featureObject.  ' +
                                 'Have no presence to bind feature ' +
                                 'changes to.');
            }
            presToChange = system.self;
        }

        if (!(presToChange.toString() in presencesToFeatureObjectMap ))
        {
            throw new Error('Error in beginFeatureTransaction.  Invalid '+
                            'presence to bind changes to.');
        }
        var featureObject = presencesToFeatureObjectMap[presToChange.toString()];
        return new std.featureObject.FeatureChangeTransaction(
            presToChange,featureObject.updateCount);
    };

    /**
     @param {std.featureObject.FeatureChangeTransaction} featChangeTrans
     @throws Error if changes to feature vector are stamped after
     latest update to vector.
     */
    std.featureObject.commitFeatureChanges =function(featChangeTrans)
    {
        if (!(featChangeTrans.pres.toString() in presencesToFeatureObjectMap))
        {
            throw new Error('Incorrect presence detected ' +
                            'when committing feature changes.');
        }
        
        var featElement = presencesToFeatureObjectMap[featChangeTrans.pres.toString()];

        //checking that all existing elements in data map
        //have not been modified since commit began.
        var addedDataMap ={};

        for (var s in featChangeTrans.actions)
        {
            var act = featChangeTrans.actions[s];
            
            if (act.type == std.featureObject.Action.ADD)
                addedDataMap[act.index] = true;
            
            if (act.index in featElement.dataCommitMap)
            {
                if (featElement.dataCommitMap[act.index] >
                    featChangeTrans.commitNum)
                {
                    throw new Error('Aborting feature transaction: ' +
                                    'detected out of date data.');
                }
            }
            else if (!act.index in addedDataMap)
            {
                throw new Error ('Aborting feature transaction: ' +
                                 'attempting operation on removed'+
                                 ' or non-existent data.');
            }

            
            if ((act.type != std.featureObject.Action.ADD)    &&
                (act.type != std.featureObject.Action.REMOVE) &&
                (act.type != std.featureObject.Action.CHANGE))
            {
                throw new Error('Unknown action type when commiting ' +
                                'feature.  Aborting.');
            }
        }

        //we know that the commit will go through.  We'll label all
        //data in commit as having been modified at updateCount.
        ++featElement.updateCount;
        
        //If gotten to this point, know we're not operating on stale
        //data.  Go ahead and commit all changes.
        for (var s in featChangeTrans.actions)
        {
            var act = featChangeTrans.actions[s];
            if ((act.type == std.featureObject.Action.ADD) ||
                (act.type == std.featureObject.Action.CHANGE))
            {
                featElement.data[act.index] = act.data;
                featElement.dataCommitMap[act.index] = featElement.updateCount;
            }
            else if (act.type == std.featureObject.Action.REMOVE)
            {
                if (act.index in featElement.data)
                {
                    delete featElement.data[act.index];
                    delete featElement.dataCommitMap[act.index];
                }
            }
        }
        sendFeaturesToSubscribers(featElement,featChangeTrans.pres);
    };

    
    /**
     If subscriber does not ack feature data after some time, then we
     unsubscribe them.
     */
    function sendFeaturesToSubscribers(featElement,pres)
    {
        
        //see handleFeatureObjUpdates in proxManager.em for format of
        //featureObject update messages.
        var updateMsg =
            {
                featureObjUpdate: featElement.data
            };
        
        //for now, ship full vector to all subscribing
        for (var s in featElement.subscribers)
        {
            pres #
                updateMsg >>
                featElement.subscribers[s] >>
                [function(){}, TIME_TO_ACK_UPDATE,
                 //if don't receive an ack, then unsubscribe the subscriber.
                 function()
                 {
                     if (s in featElement.subscribers)
                     {
                         var unsubscribeNotice =
                             {
                                 featureObjectUnsubscribe:''
                             };

                         //tell them they're being unsubscribed
                         pres  #
                             unsubscribeNotice >>
                             featElement.subscribers[s] >>[];

                         //actually remove them from subscribers
                         delete featElement.subscribers[s];
                     }
                 }];
        }
    }
})();
    
