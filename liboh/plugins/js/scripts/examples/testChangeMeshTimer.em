//This script should change a presence's mesh every 3 seconds.
//You may want to check out the testRepeatingTimer.em script
//to get a sense of how repeating timers work.


//include the library containing the repeating timer code.
system.require('std/core/repeatingTimer.em');


var originalMesh   = system.presences[0].getMesh();
//the mesh to change to will just be a white sphere
var meshToChangeTo = "meerkat:///test/sphere.dae/original/0/sphere.dae";
var onOriginalMesh = true;


//every 3 seconds, calls the callback_change_mesh function
var repTimer = new std.core.RepeatingTimer(3,callback_change_mesh);

function callback_change_mesh()
{
    system.print("\n\nGot into callback change mesh.  Changing the mesh of the presence.\n\n");
    if (onOriginalMesh)
    {
        //if presence[0] has its original mesh, change it to sphere
        onOriginalMesh = false;
        system.presences[0].setMesh(meshToChangeTo);
    }
    else
    {
        //if presence[0] has a sphere mesh, change it to its original mesh
        onOriginalMesh = true;
        system.presences[0].setMesh(originalMesh);
    }
}



