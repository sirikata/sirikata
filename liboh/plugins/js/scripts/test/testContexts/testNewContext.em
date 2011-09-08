

system.import('test/testContexts/baseContextTest.em');

x = 5;

function toExecute()
{
    if (typeof(x) == 'undefined')
        x = 7;
    else
        system.print('\n\n\nError.  Did not receive undefined x.\n\n');
    
    system.print("\n\nInside of toExecute\n\n");
    system.print("This is x: "+ x.toString() + "\n");
};

newContext.execute(toExecute);

function printXMultiple()
{
    var xAsString = x.toString();
    system.print("This is x: " + xAsString + "\n");
    system.timeout(5, printXMultiple);
}


system.timeout(5, printXMultiple);