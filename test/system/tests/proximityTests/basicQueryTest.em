
system.require('proxUtil.em');

mTest = new UnitTest('basicQueryTest');

// Test picking up objects connected before and after the
// querier.
connectObject(
    <0,0,0>,
    function() {
        connectDefaultQuerier(
            function() {
                // Wait until we have the one connection
                waitForResults(
                    mTest, 1, 5, 'Failed to get proximity result for object connected before querier',
                    function() {
                        // Then connect another object and wait for it to appear as well.
                        waitForResults(
                            mTest, 2, 5, 'Failed to get proximity result for object connected after querier',
                            function() {
                                mTest.success('Finished');
                                system.killEntity();
                            }
                        );
                        connectObject(<0,0,0>);
                    }
                );
            }
        );
    }
);
