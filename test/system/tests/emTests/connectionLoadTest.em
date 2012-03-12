/**
  Load tests the space server + OH to ensure lots of session messages
  don't cause some sessions to fail.
  */

system.require('helperLibs/util.em');

//global data needed by functions.
mTest = new UnitTest('connectionLoadTest');
numPresences = 100;
numPresencesLeft = numPresences;

// Connect and disconnect a bunch of presences
function presCallback(pres) {
    pres.disconnect();
}

presInfo = {
    'mesh':'meerkat:///kchung25/reddot.dae/optimized/reddot.dae',
    'scale':3,
    'callback': presCallback,
    'solidAngleQuery': 4,
    'pos': new util.Vec3(15,0,23),
    'vel': new util.Vec3(0,0,0),
    'orientVel': new util.Quaternion (0,0,0,1),
    'orient': new util.Quaternion(0,0,0,1)
};

function generatePresences(start_pres, on_pres_connected_callback_handle)
{
    // Clear this callback so future presence connections don't invoke this again
    on_pres_connected_callback_handle.clear();
    // Start real test
    for (var s = 0; s < numPresences; ++s)
    {
        system.createPresence(presInfo);
    }
}


// Track disconnects so we can eventually declare success
function handleDisconnect() {
    numPresencesLeft--;
    system.print('Left: ' + numPresencesLeft + '\n');
    if (numPresencesLeft == 0) {
        mTest.success('All presences connected and disconnected');
        system.killEntity();
    }
}
system.onPresenceDisconnected(handleDisconnect);

// Start the test when we get our first presence
system.onPresenceConnected(generatePresences);