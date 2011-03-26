
system.import('test/testContexts/baseContextTest.em');

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