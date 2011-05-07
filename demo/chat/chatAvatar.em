
system.import('std/graphics/default.em');
system.import('std/graphics/chat.em');

system.onPresenceConnected( function(pres) {
                                system.print("\n\nGOT INTO ON PRESENCE CREATED\n\n");

    system.print("startupCamera connected " + pres);
    system.print(system.presences.length);
    if (system.presences.length == 1)
    {
      simulator = new std.graphics.DefaultGraphics(pres, 'ogregraphics');
      chat = new std.graphics.Chat(pres, simulator.simulator());
    }
    else
    {
      print("No Chat Window");
    }
});

system.onPresenceDisconnected( function() {
    system.print("startupCamera disconnected");
});
