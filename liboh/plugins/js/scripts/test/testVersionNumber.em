
var whichPresence = system.presences[0];
var newContext = system.create_context(whichPresence,null,true,true,true,true);


function testContext()
{
    newContext.execute(toExecute);
}


function toExecute()
{
    system.print("\n\nPrinting emerson version #: \n");
    system.print(system.getVersion());
    system.print("\n\n");
}

testContext();