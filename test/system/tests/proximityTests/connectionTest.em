
system.require('emUtil/util.em');

mTest = new UnitTest('connectionTest');

system.createPresence(
    {
        'space' : '12345678-1111-1111-1111-DEFA01759ACE',
        'pos' : <0, 0, 0>,
        'callback' : function() {
            mTest.success('Finished');
            system.killEntity();
        }
    }
);
