
var minDist = 6;
var howFastToEvadeConst = 3;
var evading = false;
var howLongToFlee = 2;

//this function returns an object that has converted the relevant position fields from strings to a vec3
function parseLocationFromObject(object)
{
    var returner = new system.Vec3(parseFloat(object.positionX),parseFloat(object.positionY),parseFloat(object.positionZ));
    return returner;
}

function distance(vecA,vecB)
{
    var diffX = vecA.x - vecB.x;
    var diffY = vecA.y - vecB.y;
    var diffZ = vecA.z - vecB.z;
    
    var returner = system.sqrt(diffX*diffX + diffY*diffY + diffZ*diffZ);

    return returner;
}

function cameraLocationCallback(object, sender)
{
    var camLocation = parseLocationFromObject(object);
    var curPosition = system.presences[0].getPosition();

    if (distance(camLocation,curPosition) < minDist)
    {
        system.print("\n\n\nSQUAWK!  SQUAWK!  Evasive maneuvers\n\n");
        evasiveManeuvers(camLocation,curPosition);
    }
}

//this function takes in two position vectors associated with the camera location and current position.
//it sets velocity to be in the opposite direction of the camera
function evasiveManeuvers(camLocation,curPosition)
{
    evading = true;
    var velocityDirection = new system.Vec3(curPosition.x-camLocation.x,curPosition.y-camLocation.y, curPosition.z-camLocation.z);
    var newVel = scalarMultVec(howFastToEvadeConst, (normalizeVector(velocityDirection)));
    system.presences[0].setVelocity(newVel);
    system.timeout(howLongToFlee,null,regularOperationCallback);
    
}

function normalizeVector(vec)
{
    var normConst = system.sqrt(vec.x*vec.x + vec.y*vec.y + vec.z*vec.z);
    return scalarMultVec(1.0/normConst,vec);
}

function scalarMultVec(scalar, vec)
{
    return new system.Vec3(scalar*vec.x, scalar*vec.y, scalar*vec.z);
}


function regularOperationCallback()
{
    evading = false;
    system.presences[0].setVelocity(new system.Vec3(0,0,0));
    
}



var cameraLocationPattern = new system.Pattern("command","locationCamera");
var cameraLocationHandler = system.registerHandler(cameraLocationPattern,null,cameraLocationCallback,null);
