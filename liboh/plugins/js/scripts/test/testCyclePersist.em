

system.require('std/shim/restore/persistService.em');


var toPersist = new Object();

toPersist.a = 'a';
toPersist.b = 'b';
toPersist.c = {
    'm':'m',
    'd': toPersist
};

//printing first object graph.
system.print('\ncycled graph\n');
system.prettyprint(toPersist);

//performing persist
var fName = 'cycledGraphNoPres';
std.persist.checkpointPartialPersist(
    toPersist,fName,
    function(success) {
        if (!success) {
            system.prettyprint('Checkpoint failed.');

            // // //deleting cycle
            // delete toPersist.c;
            // system.print('\nAfter delete\n');
            // system.prettyprint(toPersist);


            // // //re-printing object.
            // system.print('\nAfter second restore\n');
            // system.prettyprint(newCopy);

            return;
        }
        //restoring object from file
        std.persist.restoreFromAsync(fName, function(success, newCopy) {
                                         system.print('\nAfter restore\n');
                                         system.prettyprint(success);
                                         system.prettyprint(newCopy);
                                     });
    }
);
