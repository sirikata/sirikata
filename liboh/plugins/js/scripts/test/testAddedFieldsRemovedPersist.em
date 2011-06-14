

system.require('std/shim/restore/persistService.em');


var toPersist = new Object();

toPersist.a = 'a';
toPersist.b = 'b';
toPersist.c = toPersist;

//printing first object graph.
system.print('\nbefore restore\n');
system.prettyprint(toPersist);

//performing persist
var fName = 'btestPartialPersist.em.bu';
std.persist.checkpointPartialPersist(
    toPersist,fName,
    function(success) {
        if (!success) {
            system.print("checkpointPartialPersist failed.");
            return;
        }

        toPersist.d = 'ooooo';
        toPersist.c = 'e';

        //restoring object from file
        std.persist.restoreFromAsync(fName, function(success, newCopy) {
                                         //re-printing object.
                                         system.print('\nAfter restore\n');
                                         system.prettyprint(newCopy);
                                     });
    }

);
