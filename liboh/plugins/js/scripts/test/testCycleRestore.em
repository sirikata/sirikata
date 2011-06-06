system.require('std/shim/restore/persistService.em');

//performing persist
var fName = 'btestPartialPersist.em.bu';
var id = 0;

//restoring object from file
var newCopy = std.persist.restoreFrom(fName,id);
system.print('\nAfter restore\n');
system.prettyprint(newCopy);
