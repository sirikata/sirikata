system.print("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
system.print("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
system.print("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
system.print("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
system.print("\n\nINITIALIZATION\n\n");
system.print("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
system.print("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
system.print("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
system.print("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");


function timerCallback()
{
    system.print("\n\n\n\nI GOT INTO THE CALLBACK\n\n\n");
    system.update_addressable();
    var x = Object();

    broadcastNotSelf(x);

}

function broadcastNotSelf(objectToSend)
{
    for (var s=0; s < system.addressable.length; ++s)
    {
        if (system.addressable[s] != system.Self)
        {
            system.addressable[s].sendMessage(objectToSend);
            system.print("\n\n\nThinking about sending here\n\n");
        }
        else
            system.print("\n\n\nNot sending a message out because it's to me\n\n");
    }
}




system.timeout(150,null,timerCallback);
