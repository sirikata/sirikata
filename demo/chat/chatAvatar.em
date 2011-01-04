
system.import("std/library.em");

simulator = undefined;
chat = undefined;

function sendAll(msg)
{
  for(var i = 0; i < addressable.length; i++)
  {
    if(addressable[i] != Self)
    {
      msg -> addressable[i];
    }
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
    chat.invoke("write", msg.chat );
}



system.onPresenceConnected( function(pres) {
    system.print("startupCamera connected " + pres);
    system.print(system.presences.length);
    if (system.presences.length == 1)
    {
      simulator = pres.runSimulation("ogregraphics");
      chat = simulator.invoke("getChatWindow");
      chat.invoke("bind", "eventname", onChatMsgReceived);
      p = new system.Pattern("chat");
      onChatFromNeighbor <- p ;
    }
    else
    {
      print("No Chat Window");
    }
});

system.onPresenceDisconnected( function() {
    system.print("startupCamera disconnected");
});
