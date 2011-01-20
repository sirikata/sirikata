
system.print("\n\nRunning a test of fakeroot for context \n\n");


// function functionToExecuteInContext()
// {
//     fakeroot.print("\n\nExecuting print in context\n\n");
// }


function proxCallback(visibleBack)
{
    // var newContext = system.createContext(system.presences[0],
    //                                       visibleBack,
    //                                       true,
    //                                       true,
    //                                       true);

    // newContext.execute(functionToExecuteInContext);

    system.print("\n\nGot into prox callback\n\n");
    
}

system.presences[0].onProxAdded(proxCallback);
system.presences[0].setQueryAngle(.4);

