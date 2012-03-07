/**
 *  userEventTest -- tests system.event
 *  Scene.db:
 *   * Ent 1 : Anything
 */


system.require('helperLibs/util.em');

mTest = new UnitTest('userEventTest');

var numTests = 1;
var finishTest = function() {
    numTests--;
    if (numTests == 0)
        system.killEntity();
};

// Using this because finishing before the initial connection causes
// shutdown problems
system.onPresenceConnected(
    function() {

        // Just check a single deferred event
        var test1Timeout = system.timeout(
            1,
            function() {
                mTest.fail('Timed out before simple user generated event was executed');
            }
        );
        system.event(
            function() {
                test1Timeout.clear();
                finishTest();
            }
        );

    }
);
