
system.require('emUtil/util.em');

// Get number of results
var resultCount = function() {
    var set = system.getProxSet(system.self);
    var count = 0;
    for (var x in set)
        count++;
    return count;
};

var connectDefaultQuerier = function(cb) {
    system.createPresence(
        {
            'space' : '12345678-1111-1111-1111-DEFA01759ACE',
            'pos' : <0, 0, 0>,
            'solidAngleQuery' : .0000001,
            'callback' : cb
        }
    );
};

var connectObject = function(pos, cb) {
    params = {
        'space' : '12345678-1111-1111-1111-DEFA01759ACE',
        'pos' : pos
    };
    if (cb) params['callback'] = cb;
    system.createPresence(params);
};

// Set up to wait for count objects to be in the prox result set, listening on
// prox events for the current presence, or stopping and triggering a failure
// after timeout seconds. The callback is invoked either when the number of
// objects is met or at the timeout so the test can continue in either case.
var waitForResults = function(test, count, timeout, timeout_msg, cb) {
    var cleanup = function() {
        if (cancel_timeout_failure) cancel_timeout_failure.clear();
        if (prox_added_handle) prox_added_handle.clear();
        if (prox_removed_handle) prox_removed_handle.clear();
        // Doing callback immediately can cause crash because killing
        // within a proximity event handler currently looks like it's
        // broken
        system.event(cb);
    };

    var check_condition = function() {
        if (resultCount() == count)
            cleanup();
    };

    var cancel_timeout_failure = system.timeout(
        timeout,
        function() {
            test.fail(timeout_msg);
            cleanup();
        }
    );

    var prox_added_handle = system.self.onProxAdded(check_condition);
    var prox_removed_handle = system.self.onProxRemoved(check_condition);

    // Trigger one check immediately in case the condition is already
    // true (which might result in no additional events)
    check_condition();
};
