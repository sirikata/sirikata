
system.require('emUtil/util.em');

mTest = new UnitTest('connectionTest');

system.event(
    function() {
        mTest.success('Finished');
        system.killEntity();
    }
);
