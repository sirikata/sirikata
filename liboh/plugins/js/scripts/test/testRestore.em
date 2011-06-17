


system.require('std/shim/restore/persistService.em');



var objToPersist = {
    'i': 'ooo'
};

var keyName = 'key_name';

function persistCallback()
{
    objToPersist.i  = 3;
    system.print('\n\nHas been persisted\n\n');
    std.persist.restoreFromAsync(keyName,restoreCallback);
}


//lkjs;


std.persist.checkpointPartialPersist(objToPersist,keyName,persistCallback);



function restoreCallback(success, objGraph)
{
    if (success)
        system.print('\n\nSuccessful callback\n\n');

    system.print('\n\n');
    system.prettyprint(objGraph);
    system.print('\n\n');
}