/**
 *  presenceEventsTest -- tests onPresenceConnected,
 *  onPresenceDisconnected, and createPresence callback
 *
 *  Scene.db:
 *   * Ent 1 : Anything
 */


system.require('helperLibs/util.em');

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
                logEvent('onPresenceConnected');
            }
        );
        system.onPresenceDisconnected(
            function() {
                logEvent('onPresenceDisconnected');
            }
        );

        // And this actually starts the process
        system.createPresence({}, presenceConnectedCallback);
    }
);
