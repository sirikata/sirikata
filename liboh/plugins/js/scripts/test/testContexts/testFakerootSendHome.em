//This function tries to call sendHome of fakeroot.  We'll see if it works!

//also testing timer callbacks

system.print("\n\nTesting sendHome of fakeroot object");



function toExecuteInContext(fakeroot)
{
    var contFuncToRepeat =   function ()
    {
        var objToSend = new Object();
        objToSend.msg = "msg";
        fakeroot.sendHome(objToSend);
        fakeroot.print("\nGot callback here\n");
    };

    for (var s=0; s < 20; ++s)
    {
        fakeroot.timeout(s*10,null,contFuncToRepeat);            
    }
}



function proxCallback(visibleBack)
{
    var newContext = system.create_context(system.presences[0],
                                          visibleBack,
                                          true,
                                          true,
                                          true,
                                          true);

    newContext.execute(toExecuteInContext);
}

system.presences[0].onProxAdded(proxCallback);
system.presences[0].setQueryAngle(.01);