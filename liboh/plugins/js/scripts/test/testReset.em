
system.import('std/core/bind.js');
system.import('std/core/repeatingTimer.em');


function timerCallback()
{
    system.print("\nIn timer callback\n");
}

var repTimer = new std.core.RepeatingTimer(3,timerCallback);


system.set_script("system.import('std/default.em'); system.import('std/shim.em');");


system.print("\n\nOkay.  Wait a while, and then call system.reset(); The repeating message should stop, and you should still be able to script.");
