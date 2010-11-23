
function testSetPosition()
{
    var initialPosition = system.presences[0].getPosition();
    printVec3(initialPosition,"\n\n\nInitial position\n");
    var displacement = system.Vec3(1,1,1);

    system.presences[0].setPosition(addVec3(initialPosition,displacement));
    
}


function testSetOrientation()
{
    var initialPosition = system.presences[0].getOrientation();
    printQuat(initialPosition,"\n\n\nInitial orientation\n");
    var displacement = system.Quaternion(1,1,1,1);

    system.presences[0].setOrientation(addQuat(initialPosition,displacement));
}


function testSetVisual()
{
    var testMesh = "meerkat:///lily/milktruck.dae"
    //    var testMesh = "meerkat:///danielrh/cube.dae"

    system.print("\n\nChanging mesh\n\n");
    system.presences[0].setMesh(testMesh);
    
}

function testGetVisual()
{

    var visualMeshUri = system.presences[0].getMesh();
    system.print("\n\n\nMesh:");
    system.print(visualMeshUri);
    system.print("\n\n\n");
}



function testSetHandler()
{
    var returnerCallBack = function()
    {
        system.print("\n\n\nI got into returner callback\n\n");
    };

    //requires message to have field m with value o.
    var mPattern = new system.Pattern("m","o");
    
    var handler = system.registerHandler(mPattern,null,returnerCallBack,null);
}

//this function sends a message to every single message-able entity with a message that will trigger the handler set in the previous function
function testForceHandler()
{
    var objectMessage = new Object();
    objectMessage.m = "o";

    system.__broadcast(objectMessage);
    // for (var s=0; s < system.addressable.length; ++s)
    // {
    //     system.addressable[s].sendMessage(objectMessage);
    // }
}






function addVec3(a,b)
{
    var returner = system.Vec3(a.x+b.x,a.y+b.y,a.z+b.z);
    return returner;
}

function printVec3(vec,stringToPrint)
{
    system.print(stringToPrint);
    system.print("x: ");
    system.print(vec.x);
    system.print("y: ");
    system.print(vec.y);
    system.print("z: ");
    system.print(vec.z);
    system.print("\n\n");
}

function addQuat(a,b)
{
    var returner = system.Quaternion(a.x+b.x,a.y+b.y,a.z+b.z, a.w+b.w);
    return returner;
}


function printQuat(quat,stringToPrint)
{
    system.print(stringToPrint);
    system.print("x: ");
    system.print(quat.x);
    system.print("y: ");
    system.print(quat.y);
    system.print("z: ");
    system.print(quat.z);
    system.print("w: ");
    system.print(quat.w);
    system.print("\n\n");    
}