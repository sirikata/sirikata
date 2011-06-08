system.require('std/shim/restore/persistService.em');

//performing persist
var fName = 'cycledGraphNoPres';


//restoring object from file
var newCopy = std.persist.restoreFrom(fName);
system.print('\nAfter restore\n');
system.prettyprint(newCopy);
