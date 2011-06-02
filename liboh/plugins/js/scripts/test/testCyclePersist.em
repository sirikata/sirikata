

system.require('std/shim/restore/persistService.em');


var toPersist = new Object();

toPersist.a = 'a';
toPersist.b = 'b';
toPersist.c = toPersist;

//printing first object graph.
system.print('\ncycled graph\n');
system.prettyprint(toPersist);

//performing persist
var fName = 'btestPartialPersist.em.bu';
checkpointPartialPersist(toPersist,fName);


//restoring object from file
restoreFrom(fName);
system.print('\nAfter restore\n');
system.prettyprint(toPersist);


// //deleting cycle
delete toPersist.c;
system.print('\nAfter delete\n');
system.prettyprint(toPersist);

// //restoring object from file
restoreFrom(fName);


// //re-printing object.
system.print('\nAfter second restore\n');
system.prettyprint(toPersist);
