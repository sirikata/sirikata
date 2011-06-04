

system.require('std/shim/restore/persistService.em');


var toPersist = new Object();

toPersist.a = 'a';
toPersist.b = 'b';
//toPersist.c = toPersist;
toPersist.c = {
    'm':'m'
};

//printing first object graph.
system.print('\ncycled graph\n');
system.prettyprint(toPersist);

//performing persist
var fName = 'btestPartialPersist.em.bu';
var nameServe = checkpointPartialPersist(toPersist,fName);


// var id = nameServe.lookupName(toPersist);

// //restoring object from file
// var newCopy = restoreFrom(fName,id);
// system.print('\nAfter restore\n');
// system.prettyprint(newCopy);


// // //deleting cycle
// delete toPersist.c;
// system.print('\nAfter delete\n');
// system.prettyprint(toPersist);


// // //re-printing object.
// system.print('\nAfter second restore\n');
// system.prettyprint(newCopy);
