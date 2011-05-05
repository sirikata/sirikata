
function test_msg_handler(msg, sender)
{
  system.print("\n\nGot a new test message\n\n");  
  /*
  for (name in msg.value.value2)
  {
    print("\n\nmsg.value.value2." + name + "=" + msg.value.value2[name] + "\n\n");
    
  }
  */
    
  system.print("\n\n sender is "+ sender.toString() + "\n\n");
  system.print("\n\n msg.sender is " + msg.sender.toString() + "\n\n");

  var reply = new Object();
  reply.name = "reply";
  reply -> msg.sender;

  system.print("Sent the Reply to: " + msg.sender.toString() + "\n\n");

  msg.f(1);
  msg.g(2);
}

function proxAddedCallback(new_addr_obj)
{

}



system.onPresenceConnected( function(pres) {
  system.print("\n\nconnected\n\n"); 
    system.presences[0].onProxAdded(proxAddedCallback);
    test_msg_handler <- {"name":"test":};
    //test_msg_handler <- new util.Pattern("name", "test");
   // system.registerHandler(test_msg_handler, new util.Pattern("name", "test"), null);
   //system.registerHandler( test_msg_handler, new util.Pattern( "name", "test" ) , null) ;}
);
