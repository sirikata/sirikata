

//This file is going to make it so that another object can subscribe to the toMonitor variable of
//this object.  
//We will send updates to a list of subscribers whenver toMonitor changes sufficiently to warrant it (as determined by
//a function.

toMonitor           =  1;
subscriberList      = [];
subscriberScopeKeys = [];

MESSAGE_SUBJECT = BFTM_SECRET_ID;
MESSAGE_SEQNO   = 1;


//this function returns true if the old value of some variable
//is sufficiently different from the new value of that variable
//to warrant calling callback servicer
function callbackChangeToMonitor (newValue, prevValue)
{
    if (newValue != prevValue)
        return true;

    return false;
}



function changeMonitoring(newValue)
{
    var prevVal     = toMonitor;
    var toMonitor   = newValue;

    //Checks if the value has changed sufficiently for
    //us to warrant calling callbackChangeToMonito
    if (checkChangeToMonitor(newValue,prevValue))
        callbackToMonitor(toMonitor);
}



//this function is called by changeMonitoring whenever the toMonitor has changed sufficiently
//to warrant calling it.  For now, all it does is run through the subscriber list, and message
//each subscriber separately
function callbackToMonitor()
{

    
    //send to all
    for (var s=0; s < subscriberList.length; ++s)
    {
        var toSend =  createMessage(subscriberScopeKeys[s]);            
        subscriberList[s].sendMessage(toSend);
    }
        
}


function createMessage(scopeKey)
{
    var returner        = new Object();
    returner.subject    = MESSAGE_SUBJECT;
    returner.seqno      = MESSAGE_SEQNO;
    returner.val        = toMonitor;
    returner.scopeKey   = scopeKey;
    //increment message seqno in case get duplicate or missing values.
    MESSAGE_SEQNO = MESSAGE_SEQNO + 1;
    
    return returner;
}




//////////////////////////PROCESS SUBSCRIPTION REQUEST///////////////////////////
SUBSCRIPTION_REQUEST = "please_subscribe";

function setupReceiveSubscriptionRequest()
{
    var subPattern = new system.Pattern(SUBSCRIPTION_REQUEST);
    var subHandler = system.registerHandler(callback_sub_req, null, subPattern,null);
}

//sender is the object that sent the request
//if the sender does not already exist in the SUBSCRIBER_LIST, we want to add the
//
function callback_sub_req(sender,req)
{
    //check if the sender already exists in SUBSCRIBER_LIST
    //if does, then return
    for (var s=0; s < subscriberList.length; ++s)
    {
        if (subscriberList[s] == sender)
            return;
    }


    //append to subscriberList
    subscriberList.append(sender);

    //send them a new object with code that they will run to get them set up.
    var subscriberObject =   createSubscriberObject();

    sender.sendMessage(subscriberObject);
}


function createSubscriberObject()
{
    var returner = new Object;
    returner.subscriberSetupFunction= createSubscriberSetupFunction();
    return returner;
}


//note: may need to special-case the system object as self
function createSubscriberSetupFunction()
{
    //assume that toModify has a field called scopeKey
    var returner = function(sender,toModify)
    {
        toModify.monitoredData = toMonitor;
        
        lkjs;


        boxed message;

        var subscriptionFinish = new Object();
        subscriptionFinish.scopeKey = toModify.scopeKey;
        sender.sendMessage(subscriptionFinish);
        

    };

    return returner;
}