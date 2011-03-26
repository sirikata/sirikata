//bhupc wrote this a while ago to test serialization and deserialization.
//You should run this script on an entity before running test_recurs2_commandline.em.
//This side of the script sets up a message handler.  As soon as this presence receives
//a message from another presence, it tries to run the functions the msg object contained
//(in fields f and g).  If things work, you should see.  "Hello 1" and "Hello World 2" printed
//to the terminal


function test_msg_handler(msg, sender)
{
  system.print("\n\nGot a new test message\n\n");  
  system.print("\n\n sender is "+ sender.toString() + "\n\n");
  system.print("\n\n msg.sender is " + msg.sender.toString() + "\n\n");

  var reply = new Object();
  reply.name = "reply";
  reply -> msg.sender;

  system.print("Sent the Reply to: " + msg.sender.toString() + "\n\n");

  msg.f(1);
  msg.g(2);
}





system.print("\n\nconnected\n\n"); 
test_msg_handler <- new util.Pattern("name", "test");



