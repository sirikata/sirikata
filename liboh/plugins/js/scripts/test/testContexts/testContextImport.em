//If this script works correctly, after calling it, you should see
//it print every 3 seconds the messages in fuccToReExec (with 5 as argAsString.


system.import('test/testContexts/baseContextTest.em');

function testContext()
{
    var x = new Object();
    x.x = 5;
    newContext.execute(toExecute,x,system);
}


function toExecute(argPassedIn, argPassedIn2)
{
    x = 7;

    system.import('std/core/bind.js');
    system.import('std/core/repeatingTimer.em');
    
    var funcToReExec = function()
    {
        argPassedIn2.print("Inside of toExecute\n");
        var argAsString = argPassedIn.x.toString();
        argPassedIn2.print("This was argPassedIn: " + argAsString + "\n");
        argAsString = x.toString();
        argPassedIn2.print("This is x: "+ argAsString + "\n");        
    };

    var repTimer = new std.core.RepeatingTimer(3,funcToReExec);

};

testContext();