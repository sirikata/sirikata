
//this variable contains all the proxy locations of nearby objects
allPartnerLocations = new Object();


//runs through the allPartnerLocations map and prints all members' x positions
function printAllPartnerLocations()
{
    system.print("\n\nPrinting data from allPartnerLocations\n\n");
    for (var s in allPartnerLocations)
    {
        system.print(allPartnerLocations[s].positionX);            
    }

    system.print("Done printing\n\n");
}


function getAveragePosition()
{
    var currentCenter = new system.Vec3(0,0,0);
    
    var numPartners = 1; //counting myself
    for (var s in allPartnerLocations)
    {
        ++numPartners;
        currentCenter.x += allPartnerLocations[s].positionX;
        currentCenter.y += allPartnerLocations[s].positionY;
        currentCenter.z += allPartnerLocations[s].positionZ;
    }

    var mCenter = system.presences[0].getPosition();
    currentCenter.x += mCenter.x;
    currentCenter.y += mCenter.y;
    currentCenter.z += mCenter.z;

    currentCenter.x = currentCenter.x/numPartners;
    currentCenter.y = currentCenter.y/numPartners;
    currentCenter.z = currentCenter.z/numPartners;


    
    return currentCenter;
}

function runTimerFunction()
{
    system.timeout(3,null,timerCallback);
}

function timerCallback()
{
    system.print("\n\nGot into timerCallback function\n\n");
    requestAllLocations();
    resetVelocity();
    runTimerFunction();
}


var velocityScaler = .05;
function getVelocityVector()
{
    var avgCenter = getAveragePosition();
    var curPos = system.presences[0].getPosition();

    printVec3(curPos,"\nThis is my current position: \n");
    
    var velocityVector = new system.Vec3(avgCenter.x - curPos.x, avgCenter.y - curPos.y, avgCenter.z - curPos.z);

    velocityVector.x *=velocityScaler;
    velocityVector.y *=velocityScaler;
    velocityVector.z *=velocityScaler;
    
    return velocityVector;
}

function resetVelocity()
{
    var newVel = getVelocityVector();
    system.presences[0].setVelocity(newVel);
    system.print("\n\nResetting velocity\n\n");
    printVec3(newVel,"\nNewVeloc vector: \n");

    var oldVel = system.presences[0].getVelocity();
    printVec3(oldVel,"\nOldVelocVector:\n");
}

//register handlers for a getLocation command
function getLocationCallback(object,sender)
{
    system.print("\n\n\nInside of getLocationCallback\n\n");
    var locMessage = new Object();
    locMessage.command = "locationResponse";


    //position
    var curPosition = system.presences[0].getPosition();
    
    locMessage.positionX = curPosition.x;
    locMessage.positionY = curPosition.y;
    locMessage.positionZ = curPosition.z;


    //orientation
    var curOrientation = system.presences[0].getOrientation();

    locMessage.orientationX = curOrientation.x;
    locMessage.orientationY = curOrientation.y;
    locMessage.orientationZ = curOrientation.z;
    locMessage.orientationW = curOrientation.w;


    //velocity
    var curVelocity = system.presences[0].getVelocity();

    locMessage.velocityX = curVelocity.x;
    locMessage.velocityY = curVelocity.y;
    locMessage.velocityZ = curVelocity.z;
    
    sender.sendMessage(locMessage);
}


var getLocationPattern = new system.Pattern("command","getLocation");
var getLocationHandler = system.registerHandler(getLocationPattern,null,getLocationCallback,null);



function requestAllLocations()
{
    var locRequest = new Object();
    locRequest.command = "getLocation";

    broadcastNotSelf(locRequest);
}


//This function sends a message to all objects that are not self.
function broadcastNotSelf(objectToSend)
{
    for (var s=0; s < system.addressable.length; ++s)
    {
        if (system.addressable[s] != system.Self)
            system.addressable[s].sendMessage(objectToSend);                
        else
            system.print("\n\n\nNot sending a message out because it's to me\n\n");
    }
}



//register handlers for a locationResponse message
function locationResponseCallback(object,sender)
{
    allPartnerLocations[sender.toString()] = copyLocationResponseObject(object);
}

//creates a new object out of a locationResponse object
function copyLocationResponseObject(object)
{
    var returner = new Object();

    //position functions
    returner.positionX = parseFloat(object.positionX);
    returner.positionY = parseFloat(object.positionY);
    returner.positionZ = parseFloat(object.positionZ);

    //velocity values
    returner.velocityX = parseFloat(object.velocityX);
    returner.velocityY = parseFloat(object.velocityY);
    returner.velocityZ = parseFloat(object.velocityZ);

    //orientation values
    returner.orientationX = parseFloat(object.orientationX);
    returner.orientationY = parseFloat(object.orientationY);
    returner.orientationZ = parseFloat(object.orientationZ);
    returner.orientationW = parseFloat(object.orientationW);
    
    return returner;
}


var locationResponsePattern = new system.Pattern("command","locationResponse");
var locationResponseHandler = system.registerHandler(locationResponsePattern,null,locationResponseCallback,null);


// lkjs;
// system.setTimeout(1,null,positionPoller);



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