system.import("std/library.em");
system.import('std/escape.em');

system.import('std/graphics/default.em');

simulator = undefined;
chat = undefined;
chat_group = new Array();


function sendAll(msg)
{
  for(var i = 0; i < chat_group.length; i++)
  {
    msg -> chat_group[i];
  }
}

function ChatMsgObject(msg)
{
  this.chat = msg;
}

function onChatMsgReceived(msg)
{
  system.print("\n\n"); 
  system.print("Got message: ");
  system.print(msg);
  system.print("\n\n"); 
  system.print("Sending out to others....\n\n");
  sendAll(new ChatMsgObject(msg));
}

function onChatFromNeighbor(msg, sender)
{
    print("Got chat: " + msg.chat + ": from neigbor " + sender);
    // FIXME escape string
    chat.invoke('eval', 'addMessage(' +  Escape.escapeString(msg.chat, '"') + ')' );
}

function handleNewChatNeighbor(msg, sender)
{
  print("Got a new entity into the chat group");
  // add this new member of the chat group
  //check for duplicats
  for(var i = 0; i < chat_group.length; i++)
  {
    if(chat_group[i].toString() == sender.toString())
    {
      return;
    }
  }

  chat_group.push(sender);
  var p = new util.Pattern("chat"); 
  onChatFromNeighbor <- p <- sender;
  
}

// arg here is an addressable object
function proxAddedCallback(new_addr_obj)
{
  if(system.Self.toString() == new_addr_obj.toString())
  {
    print("\n\nGOT SELF in the proximity update\n\n");
    return;
  }
  print("Got a new entity in proximity");
  var test_msg = new Object();
  test_msg.name = "get_protocol";
  
  //also register a callback
  var p = new util.Pattern("protocol", "chat");
  handleNewChatNeighbor <- p <- new_addr_obj;
  test_msg -> new_addr_obj;
}


function onTestMessage(msg, sender)
{
  var reply = {"protocol":"chat"};
  reply -> sender;
}


system.onPresenceConnected( function(pres) {
                                system.print("\n\nGOT INTO ON PRESENCE CREATED\n\n");
                                
    system.print("startupCamera connected " + pres);
    system.print(system.presences.length);
    if (system.presences.length == 1)
    {
        simulator = new std.graphics.DefaultGraphics(pres, 'ogregraphics');
      chat = simulator.invoke("createWindowFile", "chat_terminal", "chat/prompt.html");
      chat.invoke("bind", "event", onChatMsgReceived);
      var p  = new util.Pattern("name", "get_protocol");
      onTestMessage <- p ;
      system.presences[0].onProxAdded(proxAddedCallback);

    }
    else
    {
      print("No Chat Window");
    }
});

system.onPresenceDisconnected( function() {
    system.print("startupCamera disconnected");
});
