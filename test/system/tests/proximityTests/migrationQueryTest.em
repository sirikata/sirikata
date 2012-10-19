
system.require('proxUtil.em');

mTest = new UnitTest('migrationQueryTest');

// Place objects on both servers, connect to one server and observe objects,
// then connect to other and observe objects.
connectObject(
    <-2,0,0>, // On server 1 when distributed
    function() {
        connectObject(
            <2,0,0>, // On server 2 when distributed
            function() {
                // Connects on server 1
                connectDefaultQuerier(
                    function() {
                        // Wait until we have the one connection
                        waitForResults(
                            mTest, 2, 5, 'Failed to get proximity result for object connected before querier',
                            function() {
                                // Then migrate and wait for objects to be
                                // readded. We can't use waitForResults since
                                // the objects would be in the set
                                // already. Instead, we should actually see
                                // additions again.
                                system.self.position = <1, 0, 0>;
                                var presence_ids = {};
                                system.print("Self presence " + system.self.toString() + "\n");
                                for(var pres_idx in system.presences) {
                                    // Skip system.self because we won't get notified of it
                                    if (system.presences[pres_idx] == system.self) continue;
                                    system.print("Have presence " + system.presences[pres_idx].toString() + "\n");
                                    presence_ids[system.presences[pres_idx].toString()] = 1;
                                }
                                var added_presence_ids = {};
                                var checkAddedBackPresences = function(pres) {
                                    system.print("Added presence " + pres.toString() + "\n");
                                    added_presence_ids[pres.toString()] = 1;
                                    for(var pres_id in presence_ids) {
                                        if (added_presence_ids[pres_id] === undefined)
                                            return;
                                    }
                                    mTest.success('Finished');
                                    system.killEntity();
                                };
                                system.self.onProxAdded(checkAddedBackPresences);
                            }
                        );
                    }
                );
            }
        );
    }
);
