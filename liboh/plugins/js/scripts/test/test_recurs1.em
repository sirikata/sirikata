
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
  /*
  test_msg.value = new Object();
  test_msg.value.field1 = "field1";
  test_msg.value.field2 = "field2";
  test_msg.value.value2 = new Object();
  test_msg.value.value2.field1 = "value.value2_field1";
*/
  test_msg.sender = system.Self;
  test_msg.f = function(x) { system.print("\n\n Hello " + x +"\n\n") ;};
  test_msg.g = function(x) { system.print("\n\n Hello Hello" + x +"\n\n") ;};
  reply_handler << new util.Pattern("name", "reply") << new_addr_obj;
  test_msg -> new_addr_obj;
  system.print("MESSAGE SENT");
}


system.onPresenceConnected( function(pres) {
    system.print("\n\nconnected\n\n"); 
    system.presences[0].onProxAdded(proxAddedCallback);
    system.print("system.presences[0] = " + system.presences[0]);
    system.presences[0].getSimulation();
    var vec = new util.Vec3(1,2,3);

    }
  );
