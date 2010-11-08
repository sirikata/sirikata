

//assumes connect to space has already happened.

//HIGH LEVEL DESCRIPTION:
//When receive a message with a change mesh field, then we fire a handler for that message
//the handler changes the mesh to a pre-specified new mesh.  When, receive that message again,
//we change back to the original mesh.


originalMesh = system.presences[0].getMesh();
meshToChangeTo = " http://www.sirikata.com/content/assets/tetra.dae";
onOriginalMesh = true;

function callback_change_mesh()
{
    if (onOriginalMesh)
    {
        onOriginalMesh = false;
        system.presences[0].setMesh(meshToChangeTo);
    }
    else
    {
        onOriginalMesh = true;
        system.presences[0].setMesh(originalMesh);
    }
        

}



var changeMeshPattern = new system.Pattern("changeMesh");

//var handler = system.registerHandler(changeMeshPattern, null, callback_change_mesh, null);
var handler = system.registerHandler(callback_change_mesh,null,changeMeshPattern,null);


//Just tees everything up so that we just have to call callChangeMesh() to get things to change.
var objer = new Object();
objer.changeMesh = true;

function callChangeMesh()
{
    system.__broadcast(objer);
}