system.import('test/testContexts/baseContextTest.em');


function toExecute()
{
    var callback = function(msg)
    {
        system.__debugPrint('\n\nGot into callback\n');
        system.__debugPrint(msg.field);
        system.__debugPrint('\n\n');
        system.sendSandbox(msg,util.sandbox.PARENT);
    };
    
    system.registerSandboxMessageHandler(callback,null,null);
}




newContext.execute(toExecute);

var msg = {
    'field': 'it worked!'
};
system.sendSandbox(msg,newContext);



var callback = function(msg)
{
    system.__debugPrint('\n\nGot into parent callback\n');
    system.__debugPrint(msg.field);
    system.__debugPrint('\n\n');
};
system.registerSandboxMessageHandler(callback,null,null);