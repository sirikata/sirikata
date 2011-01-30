//this code just receives all messages that are sent to it, and prints their contents.

system.print("This does not work correctly.  It only accepts messages with field msg of value msg.  Fix it.");

function allMessagesCallback(msg,sender)
{
    system.print("\n\nGot a message:\n");
    system.print(msg);
    system.print("\nfrom\n");
    system.print(sender);
    system.print("\n\n\n");
}


//var allMessagesPattern = new util.Pattern("myPattern");
var allMessagesPattern = new util.Pattern();
//var allMessagesPattern = new util.Pattern("msg","msg");

var allMessagesHandler = system.registerHandler(allMessagesCallback,null,allMessagesPattern,null);


