//this code just receives all messages that are sent to it, and prints their contents.

system.print("\nThis function makes it so that an entity will receive and print all messages that are sent to it.  The wrinkle is that the event handler is set within a context\n");


var whichPresence = system.presences[0];
var newContext = system.create_context(whichPresence,null,true,true,true,true);


function testContext()
{
    newContext.execute(toExecute);
}


function toExecute()
{
    var allMessagesCallback = function(msg,sender)
    {
        system.print("\n\nGot a message:\n");
        system.print(msg);
        system.print("\nfrom\n");
        system.print(sender);
        system.print("\n\n\n");
    };
    
    var allMessagesPattern = new util.Pattern();

    var allMessagesHandler = system.registerHandler(allMessagesCallback,null,allMessagesPattern,null);
}
    



testContext();