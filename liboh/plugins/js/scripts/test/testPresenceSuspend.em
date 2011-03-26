//This script should test whether presences can be suspended and resumed correctly.
//We're going to set the velocity on a presence and suspend it and resume it.
//What should happen if this is working correctly is that the presence should
//start moving in a direction for 2 seconds, then it should stop for 2 seconds,
//then it should continue, etc.

var presToSuspRes = system.presences[0];
presToSuspRes.setVelocity(new util.Vec3(.3,0,0));
isSuspended = false;
system.import('std/core/repeatingTimer.em');


//if the presence isSuspended, then 
//resume it.  Otherwise, suspend it.
function funcToReExec()
{
    if (isSuspended)
    {
        isSuspended = false;
        presToSuspRes.resume();
    }
    else
    {
        isSuspended = true;
        presToSuspRes.suspend();
    }
}

var repTimer = new std.core.RepeatingTimer(2,funcToReExec);

