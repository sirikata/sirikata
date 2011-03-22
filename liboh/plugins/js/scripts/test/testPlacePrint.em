
system.import('std/core/bind.js');
system.import('std/core/repeatingTimer.em');

//alt + s to pull up scripting window for object
//call system.import('test/testPlacePrint.em'); in scripting window
//should print position of object 1/sec
//to get it to stop.  In scripting window, type repTimer.suspend();
//to get it to start again: type repTimer.reset();

function testCallback()
{
    var mPos = system.presences[0].getPosition();
    system.print("\n\n");
    system.print("x: ");
    system.print(mPos.x);
    system.print("y: ");
    system.print(mPos.y);
    system.print("z: ");
    system.print(mPos.z);
    system.print("\n\n");
    system.print("\nReceived callback\n");
}


var repTimer = new std.core.RepeatingTimer(1,testCallback);
