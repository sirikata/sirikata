

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
checkpointPartialPersist(toPersist,fName);

toPersist.d = 'ooooo';
toPersist.c = 'e';

//restoring object from file
restoreFrom(fName);


//re-printing object.
system.print('\nAfter restore\n');
system.prettyprint(toPersist);
