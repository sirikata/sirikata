//This function demonstrates how to send messages.  You may
//need to read about repeating timers (examples/testRepeatingTimer.em)
//to understand what's going on.
//Essentially, this code broadcasts a message to all presences
//that this entity's presence can see.  The message contains
//two fields msg (with value "msg") and m (with value "random").

//Can ony really test this by running something that receives the
//message as well.  Check out testReceiveAllMessages.em and
//testReceiveMessage.em for files that will match the message
//that this code broadcasts.

//include the library containing repeating timer code.
system.require('std/core/repeatingTimer.em');


system.print("\n\nGoing to broadcast a message every 7 seconds\n\n");


//first need to discover all presences that are around me
//will store all the ones that I find in the array arrayOfVisibles.
var arrayOfVisibles = new Array();

function proxAddedCallback(objAdded)
{
    arrayOfVisibles.push(objAdded);
}

//tells the system that whenever a presence occupies
//even a really, really, really small portion of my screen
//it should notify me of that presence by calling proxAddedCallback.
system.presences[0].onProxAdded(proxAddedCallback);
system.presences[0].setQueryAngle(.0000001);


//Every 7 seconds, call broadcastIt, which runs through the entire
//array of discovered presences nad sends them messages.
var repTimer = new std.core.RepeatingTimer(7,broadcastIt);

function broadcastIt()
{
    var toBroadcast = {
        "msg": "msg",
        "m": "random"
    };

    
    for (var s=0; s < arrayOfVisibles.length; ++s)
    {
        //send message "toBroadcast" to presence in
        //arrayOfVisibles[s].
        toBroadcast -> arrayOfVisibles[s];            
    }
}






