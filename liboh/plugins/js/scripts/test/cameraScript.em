
//in seconds: relatively slow to start with.
var messageSendingInterval = 2;


function broadcastPosition()
{
    var curPosition = system.presences[0].getPosition();

    var positionObjectToSend = new Object();

    system.print("<<Sending camera location>>");
    positionObjectToSend.command   = "locationCamera";
    positionObjectToSend.positionX = curPosition.x;
    positionObjectToSend.positionY = curPosition.y;
    positionObjectToSend.positionZ = curPosition.z;

    system.__broadcast(positionObjectToSend);

    system.timeout(messageSendingInterval,null,broadcastPosition);
}


function updateAndBroadcastCallback()
{
    system.update_addressable();
    broadcastPosition();    
}


system.timeout(130,null,updateAndBroadcastCallback);


