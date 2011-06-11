

if (typeof(std) == 'undefined')
    std = { };

if (typeof(std.messaging) != 'undefined')
    throw 'Error.  Already included messaging library.  Aborting instead of re-including.';


/**
 General idea: We want to support message pattern where we send a message as
 well as wait for a response for some time.  If we receive response within that
 time, we want to fire a user-defined "response" callback for that function.  If
 we do not receive a response within that time, then we fire a user-defined "no
 response" callback.
 */


(function()
{
    std.messaging ={};

    //FIXME: May want to build in some form of garbage collection for
    //the seqNumMap;

    //This keeps track of what sequence number we're on for each
    //sender, receiver pair.  Its keys are the sender.toString() +
    //receiver.toString().  Its values are the sequence number that
    //you should send on your next message.
    var seqNumMap = { };


    /**
     @param sender is the local presence that is going to send the
     message.
     @param receiver is the external presence (visible) that is
     supposed to receive the message.

     @return Returns a sequence number that we have not yet used for
     this connection.
     */
    function genSeqNumber(sender, receiver)
    {
        var key = sender.toString() + '--' + receiver.toString();

        if (!(key in seqNumMap))
            seqNumMap[key] = 0;

        var returner = seqNumMap[key];
        
        //this is a heuristic.  The basic idea is that if you send
        //more than one new message to a receiver, you don't want to
        //have the listening handlers that you set up overlap.
        //Roughly, putting 10 here says that all handlers for an alternate
        //stream will be closed by the time that you've sent 10
        //messages back and forth.  Note: may want to expand it.
        seqNumMap[key]+= 10;
        return returner;
    }


    /**
     Map.  Keys: receiver.toString + '---' + sender.toString + '|||' +
     seqNo.toString(); Values: An array.  First field of array contains the
     handler registered to listen for the response.  The second field contains
     the timeout registered to fire if haven't received a response in a certain
     period of time.
     */
    var openHandlers ={   };

    /**
     @param {string} recString the string address representing the receiver of
     the message that we're sending.
     @param {string} senderString the string address representing the sender of
     the message that we're sending.
     @param {int} seqNo. The sequence number that we're sending the message off
     with.

     @return Returns a key contructed from these three values that can be used
     to access the respHandler and onNoRespTimer stored in openHandlers.
     */
    function generateKey(recString,senderString,seqNo)
    {
        return recString + '---' + senderString + '|||' + seqNo.toString();
    }

    /**
     For description of recString,senderString, and seqNo, @see generateKey
     documentation.

     @param {handler} respHandler Handle to handler registered to fire when
     receive response message.

     @param {timeout} onNoRespTimer.  Handle to timeout registered to fire if
     haven't received a response within the proscribed time.

     Registers the key generated from recString,senderString,seqNo with value of
     a two-long array containing handles to the respHandler and onNoRespTimer.
     */
    function addOpenHandler(recString,senderString, seqNo,respHandler,onNoRespTimer)
    {
        var key = generateKey(recString,senderString,seqNo);
        openHandlers[key] = [respHandler,onNoRespTimer];
    }

    /**
     @see generateKey for recString,senderString, and seqNo.

     This cancels the onNoRespTimer and respHandler associated with the key
     generated from recString,senderString, and seqNo.  It also removes the key
     from openHandlers.
     */
    function cancelOpenHandler(recString,senderString,seqNo)
    {
        var key = generateKey(recString,senderString,seqNo);
        if (! (key in openHandlers))
            throw 'Error in successResponse.  Do not have key ' + key + ' in openHandlers.';

        
        //clear success handler
        openHandlers[key][0].clear();
        //clear timeout handler
        openHandlers[key][1].clear();

        delete openHandlers[key];
    }



    /**
     @param {object} msg.  Message we want to send to receiver from sender
     @param {visible} receiver.  The future recipient of msg and.
     @param {presence} sender.  The sender of msg.
     @param {function} onResp.  A function that executes if the receiver
     responds to the message.  The function takes two parameters: the new
     message received and the sender of that message (receiver).
     @param {float} timeToWait.  The amount of time to wait before
     de-registering the onResp handler and triggering the onNoResp function.
     @param {function} onNoResp.  A function to execute if timeToWait seconds
     have gone by and we haven't received a response to our message.
     @param {object} (optional) oldMsg.  The message that sendder received from
     receiver.  If provided, msg will be a response to oldMsg (will have next
     seqNo).
     
     */
    std.messaging.sendMessage = function (msg,receiver,sender, onResp, timeToWait,onNoResp, oldMsg)
    {
        if ('seqNo' in msg)
            throw 'Error.  Will not accept a messasge to send that already has a sequence number.';
        
        if ((typeof(oldMsg) != 'undefined') && ('seqNo' in oldMsg) &&(typeof(oldMsg.seqNo) == 'number'))
                msg.seqNo = oldMsg.seqNo + 1;  //generate the seqNo as a response to previous message.
        else
            msg.seqNo = genSeqNumber(sender,receiver); //generate freshSeqNo.

        //actually send the message.
        system.sendMessage(sender,msg,receiver);

        var recString = receiver.toString();
        var senderString = sender.toString();
        var seqNo = msg.seqNo;
        var wrapOnResp = function(msgRec,sndr)
        {
            cancelOpenHandler(recString,senderString,seqNo);
            onResp(msgRec,sndr);
        };

        var wrapOnNoResp = function()
        {
            cancelOpenHandler(recString,senderString,seqNo);
            onNoResp();
        };
        
        var respHandler    = system.registerHandler(wrapOnResp,{'seqNo': msg.seqNo+1: }, receiver );
        var onNoRespTimeout = system.timeout(timeToWait,wrapOnNoResp);

        addOpenHandler(recString,senderString,seqNo,respHandler,onNoRespTimeout);
        
    };
    
}
)();

