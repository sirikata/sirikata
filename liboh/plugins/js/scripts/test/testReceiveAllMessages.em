//this code just prints all messages that are sent to it.
//You can use testSendMessageVisible.em from a different
//entity to test if it works.

system.print("\nThis function makes it so that an entity will receive and print all messages that are sent to it.\n");

//tells the system that any message received should trigger
//the allMessagesCallback handler.  (See testReceiveMessage
//for an example of how to match messages more precisely.)
allMessagesCallback <- null;

function allMessagesCallback(msg,sender)
{
    system.print("\n\nGot a message:\n");
    system.print(msg);
    system.print("\nfrom\n");
    system.print(sender);
    system.print("\n\n\n");
}






