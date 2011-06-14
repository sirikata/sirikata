system.require('std/shim/restore/persistService.em');

//performing persist
var fName = 'cycledGraphNoPres';


//restoring object from file
std.persist.restoreFromAsync(fName, function(success, newCopy) {
                                 system.print('\nAfter restore\n');
                                 system.prettyprint(newCopy);
                             });
