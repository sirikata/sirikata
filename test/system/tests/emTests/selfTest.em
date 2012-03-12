/**
 *  selfTest -- tests system.self values/restoration after various events
 *  Scene.db:
 *   * Ent 1 : Anything
 */


system.require('helperLibs/util.em');
system.require('std/http/http.em');
system.require('std/core/bind.em');

mTest = new UnitTest('selfTest');

// Helper that sets up a test which is a function -- it executes the
// function, changes self (undefined or user specified value) and
// setups up a timeout to indicate failure. The function should invoke
// its one parameter when it is complete (to cancel the timeout).
// Also keeps track of the number of tests and kills the entity when
// done. Calling run() on the returned object executes the test and,
// if any tests are left, invokes an (optional) callback provided to
// it.
var numTests = 0;
var test = function(name, before, dur, newself) {
    numTests++;
    return function(cb) {
        // For debugging purposes
        system.__debugPrint('Staring subtest ' + name + '\n');

        var finishTest = function() {
            numTests--;
            if (numTests == 0)
                system.killEntity();
            else
                if (cb) cb();
        };

        if (!dur) dur = 5;
        var testTimeout = system.timeout(
            dur,
            function() {
                mTest.fail('Timeout reached before end of "' + name + '" test');
                finishTest();
            }
        );
        before(
            function() {
                testTimeout.clear();
                finishTest();
            }
        );
        // Note argument counting rather than checking for undefined since
        // they might explicitly specify undefined
        var otherSelf = (arguments.length < 3) ? undefined : newself;
        system.changeSelf(undefined);
    };
};

userEvent = test(
    'user generated event', function(cb) {
        var oldSelf = system.self;
        system.event(
            function() {
                if (oldSelf != system.self)
                    mTest.fail('system.self different after user generated event');
                cb();
            }
        );
    }
);

httpEvent = test(
    'http event', function(cb) {
        var oldSelf = system.self;
        std.http.basicGet(
            'http://www.google.com',
            function() {
                if (oldSelf != system.self)
                    mTest.fail('system.self different after http event');
                cb();
            }
        );
    }
);

setRestoreScriptEvent = test(
    'setRestoreScript event', function(cb) {
        var oldSelf = system.self;
        system.setRestoreScript(
            'system.import("foo.em");',
            function() {
                if (oldSelf != system.self)
                    mTest.fail('system.self different after setRestoreScript event');
                cb();
            }
        );
    }
);

disableRestoreScriptEvent = test(
    'disableRestoreScript event', function(cb) {
        var oldSelf = system.self;
        system.disableRestoreScript(
            function() {
                if (oldSelf != system.self)
                    mTest.fail('system.self different after disableRestoreScript event');
                cb();
            }
        );
    }
);

timeoutEvent = test(
    'timeout event', function(cb) {
        var oldSelf = system.self;
        system.timeout(
            0.1,
            function() {
                if (oldSelf != system.self)
                    mTest.fail('system.self different after timeout event');
                cb();
            }
        );
    }
);

loadMeshEvent = test(
    'loadMesh event', function(cb) {
        var oldSelf = system.self;
        system.self.mesh = 'meerkat:///jterrace/multimtl.dae/optimized/multimtl.dae';
        system.self.loadMesh(
            function() {
                if (oldSelf != system.self)
                    mTest.fail('system.self different after loadMesh event');
                cb();
            }
        );
    },
    15 // longer duration in case loading takes awhile
);

presenceEvents = test(
    'presence event', function(cb) {
        var oldSelf = system.self;

        system.onPresenceConnected(
            function(pres) {
                if (system.self != pres)
                    mTest.fail('system.self different from onPresenceConnected presence');
            }
        );

        system.onPresenceDisconnected(
            function(pres) {
                if (system.self != pres)
                    mTest.fail('system.self different from onPresenceDisconnected presence');
                cb();
            }
        );

        system.createPresence(
            {},
            function(pres) {
                if (system.self != pres)
                    mTest.fail('system.self different from createPresence callback\'s presence');
                pres.disconnect();
            }
        );
    },
    15 // might take some time to do connect/disconnect
);


persistenceCommitEvent = test(
    'persistence commit event', function(cb) {
        var oldSelf = system.self;
        system.storageBeginTransaction();
        system.storageRead('foo');
        system.storageCommit(
            function() {
                if (oldSelf != system.self)
                    mTest.fail('system.self different after persistence event commit');
                cb();
            }
        );
    }
);
persistenceReadEvent = test(
    'persistence read event', function(cb) {
        var oldSelf = system.self;
        system.storageRead(
            'foo',
            function() {
                if (oldSelf != system.self)
                    mTest.fail('system.self different after persistence event read');
                cb();
            }
        );
    }
);
persistenceWriteEvent = test(
    'persistence write event', function(cb) {
        var oldSelf = system.self;
        system.storageWrite(
            'foo', 'serialized data',
            function() {
                if (oldSelf != system.self)
                    mTest.fail('system.self different after persistence event write');
                cb();
            }
        );
    }
);
persistenceEraseEvent = test(
    'persistence erase event', function(cb) {
        var oldSelf = system.self;
        system.storageErase(
            'foo',
            function() {
                if (oldSelf != system.self)
                    mTest.fail('system.self different after persistence event erase');
                cb();
            }
        );
    }
);

msgEvent = test(
    'message event', function(cb) {
        var oldSelf = system.self;
        // In this test we check both receiving messages and their timeouts by
        // sending something to ourselves and never responding.
        function handleReceive(msg, sender, receiver) {
            if (sender != system.self)
                mTest.fail('system.self not set to receiver of msg');
        };
        handleReceive << [{'msgSelfTest'::}];
        function handleNoReply() {
            if (oldSelf != system.self)
                mTest.fail('system.self different after msg timeout event');
            cb();
        }
        { 'msgSelfTest' : 'x'} >> system.self >> [function(){}, .1, handleNoReply];
    }
);

// This just sets up the chain of stages to process, invoking each one
// after the previous finishes, and makes sure we only start once we
// have finished the previous one. NOTE: You may only see one failure
// because the first step that breaks could break the rest as well.
system.onPresenceConnected(
    function() {
        bind = std.core.bind;
        userEvent(
        bind(httpEvent, undefined,
        bind(setRestoreScriptEvent, undefined,
        bind(disableRestoreScriptEvent, undefined,
        bind(timeoutEvent, undefined,
        bind(loadMeshEvent, undefined,
        bind(presenceEvents, undefined,
        bind(persistenceCommitEvent, undefined,
        bind(persistenceReadEvent, undefined,
        bind(persistenceWriteEvent, undefined,
        bind(persistenceEraseEvent, undefined,
        bind(msgEvent, undefined)
            )
            )
            )
            )
            )
            )
            )
            )
            )
            )
        );
    }
);