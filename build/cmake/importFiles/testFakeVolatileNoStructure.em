

//This file is going to make it so that an object follows another object.  But not too closely.  We will not use the get location functions through proxy objects, but rather do it through message-passing.
//the data to be monitored is toMonitor.  We will send updates to a list of subscribers for that by

toMonitor      =  1;
subscriberList = [];
whichUpdate    =  0;

myIDValue      = "secretBFTM_ID";



function changeToMonitorSignificant (newValue, prevValue)
{
    if (newValue != prevValue)
        return true;

    return false;
}



function callbackToMonitorSignificant()
{
    ++whichUpdate;
    
    var messageToSend = new generateUpdateMessage(toMonitor);    

    for (var s =0; s < subscriberList.len; ++s)
        system.presences[0].sendMessage(subscriberList[s],messageToSend);
}


function generateUpdateMessage(toMonitor)
{
    var returner = new Object();

    returner.value = toMonitor;
    returner.whichUpdate = whichUpdate;

    return returner;
}


function changeMonitoring(newValue)
{
    var prevVal = toMonitor;
    toMonitor   = newValue;

    //check if the change is substantial enough to meet our conditions to perform an action.
    if (changeToMonitorSignificant(newValue,prevValue))
        callbackToMonitorSignificant();
}



///////////////////////////////////////

//Create a handler and pack it up for a subscriber.
function createHandlerObjectToShip()
{
    
    
}


//create a 






    

