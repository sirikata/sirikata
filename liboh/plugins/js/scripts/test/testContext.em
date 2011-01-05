///This function tests the execute context code.  From system, create a context, print in the context, and execute from the new context

// system.print("\n\nRunning test/testContext.em.  Tests creating a context, printing from the context, executing a function with no args, and executing a function with args \n\n");
// system.print("\nCreate context\n");

var newContext = system.create_context();

newContext.print("PRINTING FROM NEW_CONTEXT");

x = 5;

function toExecute(argPassedIn, argPassedIn2)
{
    x = 7;
    argPassedIn2.print("Inside of toExecute");
    argPassedIn2.print("This was argPassedIn: " + argPassedIn.toString());
    argPassedIn2.print("This is x: "+ x.toString());
};

newContext.execute(toExecute,32,system);
newContext.execute(toExecute,32,system);

function printXOnce()
{
    system.print("This is x: " + x.toString() );
}


function printXMultiple()
{
    system.print("This is x: " + x.toString());
    system.timeout(5, null, printXMultiple);
}


system.timeout(5, null, printXMultiple);