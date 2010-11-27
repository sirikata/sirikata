system.print("\n\n\nSTARTING A CAMERA OBJECT\n\n\n");

system.onPresenceConnected( function(pres) {
    system.print("startupCamera connected " + pres);
    if (system.presences.length == 1)
        pres.runSimulation("ogregraphics");
});

system.onPresenceDisconnected( function() {
    system.print("startupCamera disconnected");
});
