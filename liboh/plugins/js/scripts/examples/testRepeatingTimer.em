//This script demonstrates how to set a repeating timer.  Every
//5 seconds, the script will print "received callback!" to the
//console.  If you want to get it to stop, from the scripting
//prompt, enter, repTimer.suspend();


//include the library containing the repeating timer code.
system.require('std/core/repeatingTimer.em');


function testCallback()
{
    system.print("\nreceived callback!\n");
}

//first argument indicates how frequently (in seconds)
//the system should execute function testCallback.
var repTimer = new std.core.RepeatingTimer(5,testCallback);

//To get timer to stop, you can call
//repTimer.suspend();