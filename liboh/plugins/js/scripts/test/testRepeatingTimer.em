//This test is supposed to determine if can make a repeating timer object
system.import('std/core/bind.js');
system.import('std/core/repeatingTimer.em');


function testCallback()
{
    system.print("\nReceived callback\n");
}


var repTimer = new std.core.RepeatingTimer(3,testCallback);


var isSuspended = repTimer.isSuspended();
system.print("\n\n");
system.print(isSuspended);
system.print("\n\n");

//in terminal try suspending and reset-ing repTimer.