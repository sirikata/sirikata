//This function is meant to check roughly two things:
//    1) That you can create a new context from within another context (nesting)
//    2) The child context cannot do things that the parent wasn't capable of.


var whichPresence = system.presences[0];
var newContext = system.create_context(whichPresence,null,true,true,true,true);


function testContext()
{
    newContext.execute(toExecute);
}


function toExecute()
{
    var newNewContext = system.create_context(null,null,true,true,true,true);

    var funcToExec = function()
    {

        system.import('std/core/bind.js');
        system.import('std/core/repeatingTimer.em');
    
        var funcToReExec = function()
        {
            system.print('\n\nGot into re-exec\n\n');
        };

        var repTimer = new std.core.RepeatingTimer(3,funcToReExec);
    };
    
    newNewContext.execute(funcToExec);
};

testContext();