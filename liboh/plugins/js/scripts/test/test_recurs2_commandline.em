//bhupc wrote this a while ago to test serialization and deserialization.
//The general idea is taht you run this script on a presence after you run
//test_recurs1_commandline.em on another presence.  This side of the script
//packages up two functions, f and g, and puts the into fields of a message.
//It then sends this message to anyone nearby.


function test_function(x)
{
            system.print("\n\n\n Hello "+ x + "\n\n\n\n");
}

function reply_handler(reply, sender)
{
  system.print("Got a REPLY from " + sender.toString() + "\n\n") ;
}


function proxAddedCallback(new_addr_obj)
{
  system.print("\n\nIn proxAddedCallback\n\n");
  if(system.presences[0].toVisible().toString() == new_addr_obj.toString())
  {
    return;
  }
  


  var test_msg = new Object();
  test_msg.name = "test";

  test_msg.sender = system.presences[0].toVisible();
  test_msg.f = function(x) { system.print("\n\n Hello " + x +"\n\n") ;};
  test_msg.g = function(x) { system.print("\n\n Hello Hello" + x +"\n\n") ;};
  reply_handler << new util.Pattern("name", "reply") << new_addr_obj;
  test_msg -> new_addr_obj;
}



system.print("\n\nconnected\n\n"); 
system.presences[0].onProxAdded(proxAddedCallback);
//make it so that this presence can see almost everything in the world.
system.presences[0].setQueryAngle(.0001);


