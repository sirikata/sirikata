
system.require('emUtil/util.em');

mTest = new UnitTest('connectionTest');

var resultCount = function() {
    var set = system.getProxSet(system.self);
    var count = 0;
    for (var x in set)
        count++;
    return count;
};

system.createPresence(
    {
        'space' : '12345678-1111-1111-1111-DEFA01759ACE',
        'pos' : <0, 0, 0>,
        'callback' : function() {
            // Currently need this because system.getProxSet isn't valid in the
            // createPresence callback (see bug #506).
            system.event(
                function() {
                    if (resultCount() != 0)
                        mTest.fail('Incorrect number of result objects');

                    mTest.success('Finished');
                    system.killEntity();
                }
            );
        }
    }
);
