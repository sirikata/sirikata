system.require("std/default.em");
system.require("villageSimple/BallonRandom.em");

var start=0

function delete_default_presence(default_presence) {
    if(start==0) {
        system.onPresenceConnected(function() {});
        default_presence.disconnect();
        init();
    }
    start = 1;
}

system.onPresenceConnected(delete_default_presence);
