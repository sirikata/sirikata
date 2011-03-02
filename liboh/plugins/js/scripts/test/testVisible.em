system.print("\n\n Testing visible calls.\n\n");

//on visible.
//this script sets the solid angle query associated with this entity.

//the first object that satisfies this query gets saved.  Then we periodically try to move closer to it.


var haveCallback = false;
var objToMvTowards;
var inverseSpeed = 3;
var CALLBACK_PERIOD = .5;
var CALLBACK_MAX_DISTANCE = 40;
var PRINT_DEBUGGING = false;


function proxCallback(calledBack)
{
    system.print("\n\nHave prox callback\n\n");
    if (! haveCallback)
    {
        var distToIt = distanceFromMeToIt(calledBack);
        var distToItString = distToIt.toString();
        system.print("\n\nThis is dist to it " + distToItString);
        if ((distToIt < CALLBACK_MAX_DISTANCE) && (distToIt != 0 ) )
        {
            system.print("\n\nSetting haveCallback\n\n");
            haveCallback   = true;
            objToMvTowards = calledBack;        
        }
    }
    else
    {
        if (calledBack.checkEqual(objToMvTowards))
        {
            objToMvTowards =   calledBack;
        }
    }
}

function proxRemovedCallback(outOfRange)
{
    if (haveCallback)
    {
        if (outOfRange.checkEqual(objToMvTowards))
        {
            haveCallback = false;
        }
    }
}

system.presences[0].onProxAdded(proxCallback);
system.presences[0].onProxRemoved(proxRemovedCallback);

function distanceFromMeToIt(it)
{
    var myPosition  = system.presences[0].getPosition();
    var itsPosition = it.getPosition();


    var diffX =     (myPosition.x - itsPosition.x);
    var diffY =     (myPosition.y - itsPosition.y);
    var diffZ =     (myPosition.z - itsPosition.z);
    
    var diffXSquared = diffX * diffX;
    var diffYSquared = diffY * diffY;
    var diffZSquared = diffZ * diffZ;
    var sumOfSquares = diffXSquared + diffYSquared + diffZSquared;

    var distance = util.sqrt(sumOfSquares);
    
    return distance;
}



firstNoLongerVisible = true;
function moveTowards(toMoveTowards)
{
    if (! toMoveTowards.getStillVisible())
    {
        if (firstNoLongerVisible)
        {
            var newVeloc = new util.Vec3(0,0,0);
            system.presences[0].setVelocity(newVeloc);
            firstNoLongerVisible = false;
        }
        return;
    }

    firstNoLongerVisible = true;    

    var posToMoveTowards = toMoveTowards.getPosition();
    var myPosition  = system.presences[0].getPosition();

    var xComponent = posToMoveTowards.x - myPosition.x;
    var yComponent = posToMoveTowards.y - myPosition.y;
    var zComponent = posToMoveTowards.z - myPosition.z;
    
    var mNewVelocity = new util.Vec3(
        xComponent/inverseSpeed,
        yComponent/inverseSpeed,
        zComponent/inverseSpeed
    );

    var posToMoveTowardsString = posToMoveTowards.toString();
    var newVelocityString = mNewVelocity.toString();
    if (PRINT_DEBUGGING)
    {
        system.print("\nMoving towards object at position: " +  posToMoveTowardsString + "     with velocity: " +  newVelocityString +  "\n");            
    }

    system.presences[0].setVelocity(mNewVelocity);
}

function printStatement(toMoveTowards)
{
    var posToMoveTowards = toMoveTowards.getPosition();
    var myPosition  = system.presences[0].getPosition();

    system.print("\n\nPrinting pos to move towards:\n");
    var posToMoveTowardsString = posToMoveTowards.toString();
    system.print(posToMoveTowardsString);
    system.print("\n\nPrinting my pos:\n");

    var myPosString = myPosition.toString();
    system.print(myPosString);
}


//actually updates the velocity of object to go twoards.
function timerCallback()
{
    if (haveCallback)
    {
        moveTowards(objToMvTowards);
        //printStatement(objToMvTowards);
    }

    //reset timer
    system.timeout(CALLBACK_PERIOD,null,timerCallback);
}


//called so that I can get outside its radius again
function moveFarAway()
{
    var myPosition = system.presences[0].getPosition();
    myPosition.x += CALLBACK_MAX_DISTANCE*8;
    system.presences[0].setPosition(myPosition);
    system.presences[0].setVelocity(new util.Vec3(0,0,0));
}


//establish timer to move towards.
system.timeout(1,null,timerCallback);

system.presences[0].setQueryAngle(.2);