//This script shows how to create a new entity.
//The new entity's presence will have the mesh pointed to by
//meshToChangeTo (a sphere) and will be placed 2 units away from
//the presence that is creating it.
//The first thing that the new entity does is it executes the function
//newEntExec on its own entity.  This function moves a newly connected
//presence for one second in the x direction.


//new entity's presence has the mesh of a sphere
var meshToChangeTo = "meerkat:///test/sphere.dae/original/0/sphere.dae";

//new entity's presence will be located two units away from creating
//presence's position
var newPos = system.self.getPosition() + <2,0,0>;


//First thing that the new entity will do after its presence connects
//to space is import the following file.
function newEntExec()
{
    system.require('std/default.em');
    var stopMoving = function()
    {
        system.self.setVelocity(<0,0,0>);
    };
    
    var onPresConnectedCB = function()
    {
        system.self.setVelocity(<1,0,0>);
        system.timeout(2,stopMoving);
    };
    
    system.onPresenceConnected(onPresConnectedCB);
    
    system.__debugPrint('\n\nGot into new entity\n\n');
}




system.createEntityScript(newPos,
                          newEntExec,
                          null,
                          3,  //what is the solid angle query that
                          //the entity's initial presence
                          //queries with.
                          meshToChangeTo
                         );
