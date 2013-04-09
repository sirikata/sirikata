
system.require('proxUtil.em');

mTest = new UnitTest('oidRepeatConnect');

// Test picking up objects connected before and after the
// querier. When distributed, querier is on server 1 (-x).
connectObject(
    <-2,0,0>, // On server 1 when distributed
    function() {
        connectDefaultQuerierWithDefaultOID(
            function() {
                // Wait until we have the one connection
                waitForResults(
                    mTest, 1, 5, 'Failed to get proximity result for object connected before querier',
                    function() {
                        // Then connect another object and wait for it to appear as well.
                        waitForResults(
                            mTest, 2, 5, 'Failed to get proximity result for object connected after querier',
                            function() {
                                disconnectDefaultQuerierWithDefaultOID(function() {
                                     connectDefaultQuerierWithDefaultOID(
                                         function() {
                                       // Wait until we have the one connection
                                          waitForResults(
                                             mTest, 2, 5, 'Failed to get proximity result for object connected before querier',
                                             function() {
                                                 disconnectDefaultQuerierWithDefaultOID(function() {
                                                     connectDefaultQuerierWithDefaultOID(
                                                        function() {
                                                        // Wait until we have the one connection
                                                         waitForResults(
                                                           mTest, 2, 5, 'Failed to get proximity result for object connected before querier',
                                                           function() {
                                                              mTest.success('Finished');
                                                              system.killEntity();
                                                           });
                                                     });
                                                  });
                                             });
                                     });
                              });
                            }
                        );
                        connectObject(<1,0,0>); // On server 2 when distributed
                    }
                );
            }
        );
    }
);

var globalDefaultQuerier=null;

var disconnectDefaultQuerierWithDefaultOID = function(cb) {
    globalDefaultQuerier.disconnect();
    cb();
};


var connectDefaultQuerierWithDefaultOID = function(cb) {
    system.createPresence(
        {
            'object' :'87654321-1111-1111-1111-ABCDEFABCDEF',//specifying the same OID each time causes the OH to hang right now... commenting this line can make the test (temporarily) pass
            'space' : '12345678-1111-1111-1111-DEFA01759ACE',
            'pos' : <-1, 0, 0>,
            'solidAngleQuery' : .0000001,
            'callback' : function(presence){system.__debugPrint("Presence connected "+presence);if (presence){globalDefaultQuerier=presence;cb(presence);}else{mTest.print("retry connection (expected occasionally)");connectDefaultQuerierWithDefaultOID(cb);}}
        }
    );
};

