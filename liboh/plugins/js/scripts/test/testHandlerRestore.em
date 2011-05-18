
function msgReceived(msg,sender)
{
    system.print('\n\nI got a message\n');
}




var mPattern =  {'m':'random':};

var x = system.registerHandler(msgReceived,mPattern,null);
var m = x.getAllData();
//system.prettyprint(m);

x.suspend();

function createNewOne()
{
    var y = system.registerHandler(m.callback,m.patterns,m.sender,m.isSuspended);
}




