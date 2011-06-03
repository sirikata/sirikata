

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
var newCopy = restoreFrom(fName);
system.print('\nAfter restore\n');
system.prettyprint(newCopy);


// //deleting cycle
delete toPersist.c;
system.print('\nAfter delete\n');
system.prettyprint(toPersist);


// //re-printing object.
system.print('\nAfter second restore\n');
system.prettyprint(newCopy);
