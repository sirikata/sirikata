///This function tests the execute context code.  From system, create a context, print in the context, and execute from the new context


system.import('test/testContexts/baseContextTest.em');


x = 5;

var  toExecute = function (fakeroot,argPassedIn, argPassedIn2)
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