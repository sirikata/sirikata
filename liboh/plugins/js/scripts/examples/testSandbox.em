system.import('std/core/repeatingTimer.em');

var newContext = system.createSandbox(system.self, 
                                      null,  //each sbox is associated with a visible object.
                                             //in this case, just associate with visible of
                                             //first arg passed in.
                                      true,  //can I send to everyone
                                      true,  //can I receive messages from everyone
                                      true,  //can I make my own prox queries
                                      true,  //can I import
                                      true,  //can I create presences
                                      true,  //can I create entities
                                      true   //can I call eval directly 
                                     );




x = '\nOutside context\n';
function callback()
{
    system.print(x);
}

var repeatingTimer  = std.core.RepeatingTimer(5, callback);


function toExecuteInsideContext()
{
    system.require('std/core/repeatingTimer.em');

    x = '\nInside context\n';
    var callback = function()
    {
        system.print(x);
    };

    var repeatingTimer = std.core.RepeatingTimer(5, callback);
}


newContext.execute(toExecuteInsideContext);

