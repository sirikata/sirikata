system.print("\n\n Testing visible calls.\n\n");

//on visible.
//this script sets the solid angle query associated with this entity.

//the first object that satisfies this query gets saved.  Then we periodically try to move closer to it.


var haveCallback = false;
var objToMvTowards;
var inverseSpeed = 10;

function proxCallback(calledBack)
{
    if (! haveCallback)
    {
        haveCallback   = true;
        objToMvTowards = calledBack;
    }
}


system.presences[0].onProxAdded(proxCallback);


function moveTowards(toMoveTowards)
{
    var posToMoveTowards = toMoveTowards.getPosition();
    var myPosition  = system.presences[0].getPosition();

    var mNewVelocity = system.Vec3(
        (posToMoveTowards.x - myPosition.x)/inverseSpeed,
        (posToMoveTowards.y - myPosition.y)/inverseSpeed,
        (posToMoveTowards.z - myPosition.z)/inverseSpeed
    );
    
    system.presences[0].setVelocity(mNewVelocity);
    system.print("\nMoving towards object tango-tango-niner-niner.\n");
}


//actually updates the velocity of object to go twoards.
function timerCallback()
{
    if (haveCallback)
    {
        moveTowards(objToMvTowards);
    }

    //reset timer
    system.timeout(1,null,timerCallback);
}


//establish timer to move towards.
system.timeout(1,null,timerCallback);
