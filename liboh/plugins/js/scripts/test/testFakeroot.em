
system.print("\n\nRunning a test of fakeroot for context \n\n");


function functionToExecuteInContext(fakeroot)
{
    //testing print
    fakeroot.print("\n\nExecuting print in context\n\n");

    //testing math
    var x;
    fakeroot.print("Test power\n");
    x = fakeroot.math.pow(2,2);
    x = x.toString();
    fakeroot.print("Should be 4: " + x + "\n");
    x = fakeroot.math.pow(2,5);
    x = x.toString();
    fakeroot.print("Should be 32: " + x + "\n");
    x = fakeroot.math.pow(3,.5);
    x = x.toString();
    fakeroot.print("Should be 1.732...: " + x + "\n");
    x = fakeroot.math.pow(5.5,5.5);
    x = x.toString();
    fakeroot.print("Should be 11803.06...: " + x + "\n");

    //testing getposition
    x = fakeroot.getPosition();
    fakeroot.print("\nThis is getposition: " + x + "\n\n");
    
}


function proxCallback(visibleBack)
{
    var newContext = system.create_context(system.presences[0],
                                          visibleBack,
                                          true,
                                          true,
                                          true);

    newContext.execute(functionToExecuteInContext);


    
}

system.presences[0].onProxAdded(proxCallback);
system.presences[0].setQueryAngle(.4);

