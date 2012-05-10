/**
 *  presenceEventsTest -- tests onPresenceConnected,
 *  onPresenceDisconnected, and createPresence callback
 *
 *  Scene.db:
 *   * Ent 1 : Anything
 */


system.require('emUtil/util.em');

mTest = new UnitTest('presenceEventsTest');

// We just want to make sure we can trigger each of the 3 events. A
// timeout will indicate if we failed.
var expectedEvents = 3;
var testTimeout = system.timeout(
    5,
    function() {
        mTest.fail('Timed out before all events were triggered.');
        system.killEntity();
    }
);

// Verify that functionality that should always be available in a presence
// callback is there
var verifyAvailableFunctionality = function() {
    // system.self should be available when we get connected
    if (system.self === undefined || system.self === null) {
        mTest.fail("system.self isn't available in presence connection event callback");
        return;
    }

    // getProxSet should work; even if we didn't register a query, it should
    // just be empty.
    try {
        var ps = system.getProxSet(system.self);
    }
    catch(e) {
        mTest.fail("Exception when trying to get prox set");
    }
};

var logEvent = function(name) {
    system.print('Callback ' + name + ' fired for ' + system.self + '\n');
    expectedEvents--;
    if (expectedEvents == 0) {
        testTimeout.clear();
        system.killEntity();
    }
};

system.onPresenceConnected(
    function(newPres, clearable) {
        clearable.clear();

        // 3 handlers
        var presenceConnectedCallback = function() {
            verifyAvailableFunctionality();
            logEvent('createPresenceCallback');
            // Delayed event, to ensure all presence connected callbacks finish
            system.event(
                function() {
                    system.self.disconnect();
                }
            );
        };
        system.onPresenceConnected(
            function() {
                verifyAvailableFunctionality();
                logEvent('onPresenceConnected');
            }
        );
        system.onPresenceDisconnected(
            function() {
                verifyAvailableFunctionality();
                logEvent('onPresenceDisconnected');
            }
        );

        // And this actually starts the process
        system.createPresence({}, presenceConnectedCallback);
    }
);
