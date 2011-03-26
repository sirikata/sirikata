
system.print("\n\nRunning a test of system for context \n\n");


function functionToExecuteInContext()
{
    //testing print
    system.print("\n\nExecuting print in context\n\n");

    //testing util
    var x;
    system.print("Test power\n");
    x = util.pow(2,2);
    x = x.toString();
    system.print("Should be 4: " + x + "\n");
    x = util.pow(2,5);
    x = x.toString();
    system.print("Should be 32: " + x + "\n");
    x = util.pow(3,.5);
    x = x.toString();
    system.print("Should be 1.732...: " + x + "\n");
    x = util.pow(5.5,5.5);
    x = x.toString();
    system.print("Should be 11803.06...: " + x + "\n");

    //testing getposition
    x = system.getPosition();
    system.print("\nThis is getposition: " + x + "\n\n");

    
    
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

    newContext.execute(functionToExecuteInContext);


    
}

system.presences[0].onProxAdded(proxCallback);
system.presences[0].setQueryAngle(.4);

