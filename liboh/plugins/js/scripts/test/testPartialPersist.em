

system.require('std/shim/restore/persistService.em');


var toPersist = new Object();

toPersist.a = 'a';
toPersist.b = 'b';

//printing first object graph.
system.print('\nBefore delete\n');
system.prettyprint(toPersist);

//performing persist
var fName = 'btestPartialPersist.em.bu';
std.persist.checkpointPartialPersist(
    toPersist,fName,
    function(success) {
        if (!success) return;

        //removing a field from toPersist object (to see if it'll come back
        //when restore).
        delete toPersist.a;
        system.print('\nAfter delete\n');
        system.prettyprint(toPersist);

        //restoring object from file
        std.persist.restoreFromAsync(fName, function(success, obj) {
                                         system.prettyprint('After restore:');
                                         system.prettyprint(obj);
                                     });
    }
);
