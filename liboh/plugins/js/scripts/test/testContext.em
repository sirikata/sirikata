///This function tests the execute context code.  From system, create a context, print in the context, and execute from the new context

system.print("\n\nRunning test/testContext.em.  Tests creating a context, printing from the context, executing a function with no args, and executing a function with args \n\n");


system.print("\nCreate context\n");

var newContext = system.create_context();

newContext.print("\n\nPRINTING FROM NEW_CONTEXT\n\n");

x = 5;

function toExecute()
{
    x =7;
    system.print("\n\nInside of toExecute\n\n");
    printXOnce();
};

newContext.execute(toExecute);

function printXOnce()
{
    system.print("\nThis is x: " + x.toString() + "\n");
}


function printXMultiple()
{
    system.print("\nThis is x: " + x.toString() + "\n");
    system.timeout(5, null, printXMultiple);
}


system.timeout(5, null, printXMultiple);