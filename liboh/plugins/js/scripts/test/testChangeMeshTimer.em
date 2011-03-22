


originalMesh = system.presences[0].getMesh();
meshToChangeTo = " http://www.sirikata.com/content/assets/tetra.dae";
onOriginalMesh = true;

function callback_change_mesh()
{
    system.print("\n\nGot into callback change mesh.  Changing the mesh of the object.\n\n");
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
    system.timeout(5,timeoutFunction);
}

function timeoutFunction()
{
    callback_change_mesh();
}

system.timeout(5,timeoutFunction);
