

function getLocationCallback(object,sender) {
   sender.sendMessage({command:"locationResponse",position:system.presences[0].getPosition(),
                                                  velocity:system.presences[0].getVelocity(),
                                                  orientation:system.presences[0].getOrientation()});
}

function getPositionCallback(object,sender) {
   sender.sendMessage({command:"positionResponse",position:system.presences[0].getPosition()});
}

function getOrientationCallback(object,sender)
{
   sender.sendMessage({command:"orientationResponse":orientation:system.presences[0].getOrientation()});
}
function getVelocityCallback(object,sender) {
   sender.sendMessage({command:"velocityResponse",velocity:system.presences[0].getVelocity()});
}

//requires message to have field m with value o.
var positionPattern = new system.Pattern("command","getPosition");
var velocityPattern = new system.Pattern("command","getVelocity");
var orientationPattern = new system.Pattern("command","getOrientation");
var locationPattern = new system.Pattern("command","getLocation");
    
var posHandler = system.registerHandler(positionPattern,null,getPositionCallback,null);
var velHandler = system.registerHandler(velocityPattern,null,getVelocityCallback,null);
var orientHandler = system.registerHandler(orientationPattern,null,getOrientationCallBack,null);
var locHandler = system.registerHandler(locationPattern,null,getLocationCallback,null);


var remoteLocations={};

function requestLocation(sender) {
   sender.sendMessage({command:"getLocation"});
}

function locationResponseCallback(object,sender) {
   delete object.command;
   remoteLocations[sender]=object;
}




function positionPoller()
{
   system.__broadcast({command:"getLocation"});
}


function boidPoller()
{
   var centroid = new system.Vec3(0,0,0);
   var numAve=0;
   for (i in remoteLocations) {
     centroid[i] = addVec3(centroid,remoteLocations[i].position);
     ++numAve;
   }
   centroid=scaleVector(centroid,1/numAve);
   centroid.y/=numAve;
   centroid.z/=numAve;
   system.presences[0].setPosition(addVector(scaleVector(centroid,.1),scaleVector(system.presences[0].getPosition(),.9)));
}



var locationResponsePattern = new system.Pattern("command","getLocationResponse");
var locResponseHandler = system.registerHandler(locationResponsePattern,null,locationResponseCallback,null);
system.setTimeout(1,null,positionPoller);
system.setTimeout(1,boidPoller);

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
    
    for (var s=0; s < system.addressable.length; ++s)
    {
        system.addressable[s].sendMessage(objectMessage);
    }
}






function addVec3(a,b)
{
    var returner = system.Vec3(a.x+b.x,a.y+b.y,a.z+b.z);
    return returner;
}
function scaleVec3(a,b)
{
    var returner = system.Vec3(a.x/b,a.y/b,a.z/b);
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