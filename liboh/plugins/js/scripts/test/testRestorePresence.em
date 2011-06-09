system.require('std/shim/restore/persistService.em');

var fName = 'presToPersist';

var newCopy = std.persist.restoreFrom(fName);
system.print('\nAfter restore\n');
system.prettyprint(newCopy);
