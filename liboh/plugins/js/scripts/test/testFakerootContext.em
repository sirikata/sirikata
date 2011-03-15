///This function tests the execute context code.  From system, create a context, print in the context, and execute from the new context

// system.print("\n\nRunning test/testContext.em.  Tests creating a context, printing from the context, executing a function with no args, and executing a function with args \n\n");
// system.print("\nCreate context\n");

var whichPresence = system.presences[0];
var newContext = system.create_context(whichPresence,null,true,true,true);


x = 5;

function toExecute(fakeroot,argPassedIn, argPassedIn2)
{
    x = 7;
    argPassedIn2.print("Inside of toExecute\n");
    var argAsString = argPassedIn.toString();
    argPassedIn2.print("This was argPassedIn: " + argAsString + "\n");
    argAsString = x.toString();
    argPassedIn2.print("This is x: "+ argAsString + "\n");
};

newContext.execute(toExecute,32,system);
newContext.execute(toExecute,32,system);


function printXOnce()
{
    var xAsString = x.toString();
    system.print("This is x: " + xAsString + "\n" );
}


function printXMultiple()
{
    var xAsString = x.toString();
    system.print("This is x: " + xAsString + "\n");
    system.timeout(5, null, printXMultiple);
}


system.timeout(5, null, printXMultiple);