system.print("\n\n Testing visible calls.\n\n");

//on visible.
//this script sets the solid angle query associated with this entity.

//the first object that satisfies this query gets saved.  Then we periodically try to move closer to it.


var haveCallback = false;
var objToMvTowards;
var inverseSpeed = 1;
var CALLBACK_PERIOD = .2;
var CALLBACK_MAX_DISTANCE = 40;


function proxCallback(calledBack)
{
    system.print("\n\nHave prox callback\n\n");
    if (! haveCallback)
    {
        var distToIt = distanceFromMeToIt(calledBack);
        system.print("\n\nThis is dist to it " + distToIt.toString());
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


system.presences[0].onProxAdded(proxCallback);

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

    
    var distance = system.math.sqrt(sumOfSquares);
    
    return distance;
}

firstNoLongerVisible = true;
function moveTowards(toMoveTowards)
{
    if (! toMoveTowards.getStillVisible())
    {
        if (firstNoLongerVisible)
        {
            system.presences[0].setVelocity(new system.Vec3(0,0,0) );
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
    
    var mNewVelocity = new system.Vec3(
        xComponent/inverseSpeed,
        yComponent/inverseSpeed,
        zComponent/inverseSpeed
    );

    system.print("\nMoving towards object at position: " +  posToMoveTowards.toString() + "     with velocity: " +  mNewVelocity.toString() +  "\n");
    system.presences[0].setVelocity(mNewVelocity);
}

function printStatement(toMoveTowards)
{
    var posToMoveTowards = toMoveTowards.getPosition();
    var myPosition  = system.presences[0].getPosition();

    system.print("\n\nPrinting pos to move towards:\n");
    system.print(posToMoveTowards.toString());
    system.print("\n\nPrinting my pos:\n");
    system.print(myPosition.toString());
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


//establish timer to move towards.
system.timeout(1,null,timerCallback);

system.presences[0].setQueryAngle(.2);