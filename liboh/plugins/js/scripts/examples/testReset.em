//This script shows how reset works, but needs manual
//intervention to test.
//After importing this script, every one second, a repeating
//timer fires a message.  After observing this behavior, after
//a while, a scripter should call 'system.reset' in the terminal.
//Doing so, wipes away all code associated with the entity (except
//whatever is contained in the string passed into the set_script
//command).  Therefore, the timer should stop.  The script that is
//set by the set_script command should re-import basic libraries
//associated with the entity, and print "Reset!".  



//import the library for a repeating timer.
system.require('std/core/repeatingTimer.em');

function timerCallback()
{
    system.print("\nIn timer callback\n");
}
//every second, print 'in timer callback'.
var repTimer = new std.core.RepeatingTimer(1,timerCallback);


//set_script takes in a string.  After reset is called,
//the entity automatically executes whatever is in this
//string.
//std/default.em and std/shim.em are basic libraries that
//most entities should be associated with.  For entities
//that are already-connected to the world, the system, automatically
//imported these libraries.  However, you must re-import
//them because you're clearing away all code associated
//with the entity.
//Last line just prints "Reset!" when done.
system.set_script("system.import('std/default.em'); system.import('std/shim.em'); system.print('\\nReset!\\n'); ");



system.print("\n\nOkay.  Wait a while, and then call system.reset(); The repeating message should stop, and you should still be able to script.");
