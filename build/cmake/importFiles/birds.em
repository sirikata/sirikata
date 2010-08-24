

//register handlers for a getLocation command
function getLocationCallback(object,sender)
{
    system.print("\n\n\nInside of getLocationCallback\n\n");
    var locMessage = new Object();
    locMessage.command = "locationResponse";
    locMessage.positionX = 32;
    //    locMessage.position = system.presences[0].getPosition();
    // locMessage.velocity = system.presences[0].getVelocity();
    // locMessage.orientation = system.presences[0].getOrientation();
    // locMessage.senderer = system.Self;

//    object.senderer.sendMessage(locMessage);
    sender.sendMessage(locMessage);
}


var getLocationPattern = new system.Pattern("command","getLocation");
var getLocationHandler = system.registerHandler(getLocationPattern,null,getLocationCallback,null);



function requestAllLocations()
{
    var locRequest = new Object();
    locRequest.command = "getLocation";
    locRequest.senderer = system.Self;
    system.__broadcast(locRequest);
}





//register handlers for a locationResponse message
function locationResponseCallback(object,sender)
{
    system.print("\n\n\nGot inside of the location response callback\n\n");
    system.print(object.positionX);
    system.print("\n\n\n");
    //printVec3(object.position);
}

var locationResponsePattern = new system.Pattern("command","locationResponse");
var locationResponseHandler = system.registerHandler(locationResponsePattern,null,locationResponseCallback,null);





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