
system.require('std/default.em');
system.require('std/shim/restore/simpleStorage.em');

/**
 Whenever someone sends a touch message to me, I create a new entity next to me.
 */
var DEFAULT_ENTITY_DIST_FROM_ME = <5,0,0>;
var DEFAULT_ENTITY_MESH        = "meerkat:///test/multimtl.dae/original/0/multimtl.dae";
var DEFAULT_ENTITY_QUERY = '';
var DEFAULT_ENTITY_SCALE       = 1;


function birthEntity()
{
    function newEntityScript()
    {
        system.require('std/default.em');
    }
    system.createEntityScript(system.self.getPosition() + DEFAULT_ENTITY_DIST_FROM_ME,
                              newEntityScript,
                              null,
                              DEFAULT_ENTITY_QUERY,
                              DEFAULT_ENTITY_MESH,
                              DEFAULT_ENTITY_SCALE
                             );
}


birthEntity << { 'action': 'touch':};    

function onRestore()
{
    system.require('test/testBirthingTower.em');
}
system.timeout(5, function(){std.simpleStorage.setScript(onRestore.toString(),false);});