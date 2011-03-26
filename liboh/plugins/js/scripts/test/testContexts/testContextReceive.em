//another entity should run the testSendMessageVisible.em script
//all this script does is ensure that this context can setup a receive handler for messages
//that match the simple pattern of having a field "msg" with value "msg".

system.import('test/testContexts/baseContextTest.em');

function testContext()
{
    newContext.execute(toExecute);
}


function toExecute()
{
    system.import('std/core/pretty.em');
    
    var callbacker = function(msg, msgSender)
    {
        system.print("\n\nGot message from sender.  Message was: \n");
        system.print(std.core.pretty(msg));
        system.print("\n\n");
        system.print("Sender was: ");
        system.print(msgSender.toString());
        system.print("\n\n");
    };
    
    callbacker <- new util.Pattern("msg", "msg");

    system.print("\n\n");
    system.print("Setting callback now");
    system.print("\n\n");
}

testContext();