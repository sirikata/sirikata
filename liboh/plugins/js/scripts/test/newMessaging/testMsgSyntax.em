
var TIME_TO_WAIT = 5;

var basicHandler = system.registerHandler(onResp,{'basic'::},null);

//If receive no response, just say so.
function onNoResp()
{
    system.print('\nGot no response on receiver!!!\n');
}



//when we receive a response to a message that we've sent, print the
//sequence number of that message, and respond back (for a few times).
function onResp(msg,sender)
{
    system.print('\n This is seqNo of new message on receiver: ' + msg.seqNo);
    var newMsg = {
        't': '1'
    };

    msg.makeReply(newMsg) >> [onResp,TIME_TO_WAIT,onNoResp];

}


