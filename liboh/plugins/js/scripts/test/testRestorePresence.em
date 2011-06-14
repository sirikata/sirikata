system.require('std/shim/restore/persistService.em');

var fName = 'presToPersist';

//var newCopy = std.persist.restoreFrom(fName);
function restoredCB(success, restObjGraph, nServ)
{
    system.print('\n\nDid I ever get into callback?\n\n');
    restObjGraph.setVelocity(new util.Vec3(1,0,0));
}

std.persist.restoreFromAsync(fName,restoredCB);
system.print('\nAfter restore\n');
