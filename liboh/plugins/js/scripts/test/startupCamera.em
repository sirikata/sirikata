system.print("\n\n\nSTARTING A CAMERA OBJECT\n\n\n");
system.presences[0].runSimulation("ogregraphics");

system.onPresenceConnected( function() {
    system.print("startupCamera connected");
});

system.onPresenceDisconnected( function() {
    system.print("startupCamera disconnected");
});
