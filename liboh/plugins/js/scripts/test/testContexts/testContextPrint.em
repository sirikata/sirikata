//this code just receives all messages that are sent to it, and prints their contents.

system.print("\nThis function makes it so that an entity will receive and print all messages that are sent to it.  The wrinkle is that the event handler is set within a context\n");

system.import('test/testContexts/baseContextTest.em');


function testContext()
{
    newContext.execute(toExecute);
}


function toExecute()
{
    system.print("\n\nInside of toExecute\n");
    system.print("\n\n\n");
}
    
testContext();


