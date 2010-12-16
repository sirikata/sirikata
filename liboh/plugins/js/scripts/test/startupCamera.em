
system.print("\n\n\nSTARTING A CAMERA OBJECT\n\n\n");

simulator = undefined;
chat = undefined;

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
  broadcast(new ChatMsgObject(msg));
}

function onChatFromNeighbor(sender, msg)
{
    print("Got chat from neigbor " + sender);
    chat.invoke("write", msg );
}



system.onPresenceConnected( function(pres) {
                                system.print("\n\nGOT INTO ON PRESENCE CREATED\n\n");
                                
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
