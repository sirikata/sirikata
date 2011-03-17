system.print("\n\nGoing to broadcast a message every 7 seconds\n\n");

var arrayOfVisibles = new Array();



function broadcastIt()
{
    var toBroadcast = new Object();
    toBroadcast.msg = "msg";
    toBroadcast.m = "random";
    
    for (var s=0; s < arrayOfVisibles.length; ++s)
    {
        arrayOfVisibles[s].sendMessage(toBroadcast);
    }
    
    system.presences[0].broadcastVisible(toBroadcast);
    system.timeout(7,broadcastIt);
}

function proxAddedCallback(objAdded)
{
    arrayOfVisibles.push(objAdded);
}


system.presences[0].onProxAdded(proxAddedCallback);

system.timeout(7,broadcastIt);
system.presences[0].setQueryAngle(.0000001);
