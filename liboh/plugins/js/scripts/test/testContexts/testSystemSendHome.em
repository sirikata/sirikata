//This function tries to call sendHome of system.  We'll see if it works!
//also testing timer callbacks

//this should be run with another object running /tests/testReceiveAllMessages.em
//If that other object receives the message that one of these contexts sends, we're doing okay.

system.print("\n\nTesting sendHome of system object");



function toExecuteInContext()
{
    var contFuncToRepeat =   function ()
    {
        var objToSend = new Object();
        objToSend.msg = "msg";
        system.sendHome(objToSend);
        system.print("\nGot callback here\n");
    };

    for (var s=0; s < 20; ++s)
    {
        system.timeout(s*10,contFuncToRepeat);
    }
}



function proxCallback(visibleBack)
{
    var newContext = system.create_context(system.presences[0],
                                           visibleBack,
                                           true,  //can I send to everyone
                                           true,  //can I receive messages from everyone
                                           true,  //can I make my own prox queries
                                           true,  //can I import
                                           true,  //can I create presences
                                           true,  //can I create entities
                                           true   //can I call eval directly 
                                          );

    newContext.execute(toExecuteInContext);
}

system.presences[0].onProxAdded(proxCallback);
system.presences[0].setQueryAngle(.01);