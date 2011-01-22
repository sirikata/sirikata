
system.print("\n\nRunning a test of fakeroot for context \n\n");


function functionToExecuteInContext(fakeroot)
{
    fakeroot.print("\n\nExecuting print in context\n\n");
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

