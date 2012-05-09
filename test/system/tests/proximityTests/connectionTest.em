
system.require('proxUtil.em');

mTest = new UnitTest('connectionTest');

connectDefaultQuerier(
    function() {
        if (resultCount() != 0)
            mTest.fail('Incorrect number of result objects');

        mTest.success('Finished');
        system.killEntity();
    }
);
