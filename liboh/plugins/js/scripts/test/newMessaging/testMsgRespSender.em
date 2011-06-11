

system.import('std/shim/wrappedSendMessage/messaging.em');


var numMsgsSent = 0;
var MAX_MESSAGES  = 10;
var TIME_TO_WAIT = 5;

//If receive no response, just say so.
function onNoResp()
{
    system.print('\nGot no response\n');
}

//when we receive a response to a message that we've sent, print the
//sequence number of that message, and respond back (for a few times).
function onResp(msg,sender)
{
    system.print('\n This is seqNo of new message: ' + msg.seqNo);

    ++numMsgsSent;
    var newMsg = {
        't': '1'
    };
    if (numMsgsSent < MAX_MESSAGES)
        std.messaging.sendMessage(newMsg,sender,system.self,onResp,TIME_TO_WAIT, onNoResp, msg);
    
}


//whenever we get a new proximate object, the first thing that we do
//is send it one of our messages.
function proxAddedCB(newVis)
{
    var testMsg = {
        'basic':'b'
    };

    std.messaging.sendMessage(testMsg, newVis,system.self,onResp,TIME_TO_WAIT, onNoResp);
}


system.self.onProxAdded(proxAddedCB);
system.self.setQueryAngle(.00001);




