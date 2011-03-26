//this code just receives all messages that are sent to it, and prints their contents.

system.print("\nThis function makes it so that an entity will receive and print all messages that are sent to it.\n");

function allMessagesCallback(msg,sender)
{
    system.print("\n\nGot a message:\n");
    system.print(msg);
    system.print("\nfrom\n");
    system.print(sender);
    system.print("\n\n\n");
}



var allMessagesPattern = new util.Pattern();


var allMessagesHandler = system.registerHandler(allMessagesCallback,null,allMessagesPattern,null);


