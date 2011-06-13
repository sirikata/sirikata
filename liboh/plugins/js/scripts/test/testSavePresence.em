


system.require('std/shim/restore/persistService.em');
var fName = 'presToPersist';
std.persist.checkpointPartialPersist(
    system.self,fName,
    function(success) {
        if (success)
            system.print("Saving presence succeeded\n");
        else
            system.print("Saving presence failed\n");
    }
);
