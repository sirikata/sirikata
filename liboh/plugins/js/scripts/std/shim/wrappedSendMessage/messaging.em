


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
    var DEFAULT_TIME_TO_WAIT = 5;
    
    std.messaging ={};

    /**
     @param {object} msgToSend.  An object that we want to send to
     receiver.
     @param{visible} receiver.  A visible that we want to receive the
     message;
     */
    function MessageReceiverPair(msgToSend,receiver)
    {
        this.msg = msgToSend;
        this.receiver = receiver;
    }

    /**
     @param {any} toCheck is any variable.
     @return {bool} Returns true if toCheck is a visible object.  Otherwise, returns false.
     */
    function checkIsVisible(toCheck)
    {
        if ((typeof(toCheck) != 'object')  || (toCheck == null))
            return false;

        if (! ('__getType' in toCheck))
            return false;

        if (typeof(toCheck['__getType']) != 'function')
            return false;

        return (toCheck.__getType() == 'visible');
    }

    function checkIsPresence(toCheck)
    {
        if ((typeof(toCheck) != 'object')  || (toCheck == null))
            return false;

        if (! ('__getType' in toCheck))
            return false;

        if (typeof(toCheck['__getType']) != 'function')
            return false;

        return (toCheck.__getType() == 'presence');        
    }
    
    /**
     @param {any} Variable to check if is a MessageReceiverPair
     object.
     
     @return {bool} Returns true if objToCheck is a
     MessageReceiverPair, returns false otherwise.
     */
    function checkIsMessageReceiverPair(objToCheck)
    {
        if ((typeof(objToCheck) != 'object') || (objToCheck == null))
            return false;
        
        return (objToCheck.constructor.toString().indexOf('function MessageReceiverPair') != -1);
    }

    /**
     @param {any} Variable to check if is a MessageReceiverSender
     object.
     
     @return {bool} Returns true if objToCheck is a
     MessageReceiverSender, returns false otherwise.
     */
    function checkIsMessageReceiverSender(objToCheck)
    {
        if ((typeof(objToCheck) != 'object') || (objToCheck == null))
            return false;
        
        return (objToCheck.constructor.toString().indexOf('MessageReceiverSender') != -1);
    }

    
    
    /**
     @param {any} toCheck.  Variable to check if it is an arrray.
     @return {bool} Returns true if toCheck is an array.  False otherwise.
     */
    function checkIsArray(toCheck)
    {
        if ((typeof(toCheck) != 'object') || (toCheck == null))
            return false;

        return (toCheck.constructor.toString().indexOf('Array') != -1);
    }

    /**
     presence : mrp;
     returns this object
     (note: can use msg>> recevier to construct mrp);
     
     @param {presence} sender: presence sending the message from.
     @param {MessageReceiverPair} mrp.  The paired message to send and
     receiver to send that message to.
     */
    function MessageReceiverSender(sender,mrp)
    {
        if (!(checkIsPresence(sender) && checkIsMessageReceiverPair(mrp)))
            throw 'Error constructing MessageReceiverSender object.  Requires sender to be presence and mrp to be a messagereceiverpair object.';
        
        this.mrp = mrp;
        this.sender = sender;
    }

    std.messaging.MessageReceiverSender = function(sender,mrp)
    {
        return MessageReceiverSender(sender,mrp);
    };

    
    /**
     presence : mrp;
     returns this object
     (note: can use msg>> recevier to construct mrp);
     
     @param {presence} sender: presence sending the message from.
     @param {MessageReceiverPair} mrp.  The paired message to send and
     receiver to send that message to.
     */
    std.messaging.MessageReceiverSender = function(sender,mrp)
    {
        if (!(checkIsPresence(sender) && checkIsMessageReceiverPair(mrp)))
            throw 'Error constructing MessageReceiverSender object.  Requires sender to be presence and mrp to be a messagereceiverpair object.';
        
        this.mrp = mrp;
        this.sender = sender;
    };

    /**
     Turns message receiver pair into a message receiver sender object
     (using system.self as sender) and then calls down to
     sendMessageReceiverSender, which performs additional type check
     on responseArray and actually sends the message.
     */
    function callSendMessageReceiverPair(mrp,responseArray)
    {
        throw '\n\nDEBUG: error, should not have proceeded to this call\n\n';
        
        var mrs = new std.messaging.MessageReceiverSender(system.self,mrp);
        return callSendMessageReceiverSender(mrs,responseArray);
    }

    /**
     Does some basic checks on the responseArray (has 3 or less
     fields, has functions in first and last slots and a number in the
     middle, etc.
     */
    function callSendMessageReceiverSender(mrs, responseArray)
    {
        if (responseArray.length > 3)
            throw 'Error: Incorrectly formatted response array.  Response array requires three arguments or fewer 1) function to call on responses to your message; 2) amount of time to wait before stop listening for response; 3) function to execute if receive no response after this time.';

        var respFunc   = function(){};
        var timeToWait = DEFAULT_TIME_TO_WAIT;
        var noRespFunc = function(){};
        if (responseArray.length >= 1)
        {
            respFunc = responseArray[0];
            if (typeof(respFunc) != 'function')
                throw 'Error: Response function must be a function';
        }
        if (responseArray.length >=2)
        {
            timeToWait = responseArray[1];
            if (typeof(timeToWait) != 'number')
                throw 'Erorr: time to wait for response must be a number';
        }
        if (responseArray.length >= 3)
        {
            noRespFunc = responseArray[2];
            if (typeof(noRespFunc) != 'function')
                throw 'Error: Third arg in response array must be a function';
        }

        return std.messaging.sendMessage(mrs.mrp.msg, mrs.mrp.receiver,mrs.sender,respFunc,timeToWait,noRespFunc);
    }
    
    /**
     lhs >> rhs;

     
     @lhs {object} message to send
     
     @rhs {visible} receiver to send message to.

     @return {MessageReceiverPair} Returns a message receiver pair
     object.  This object can later be used to actually send the
     message.  (eg. presToSendFrom : message_receiver_pair >> []; See
     alternate arguments to understand message_receiver_pair >> []; )

     
     or

     
     @lhs {MessageReceiverPair} An object that contains both the
     message to send and the receiver that is supposed to receive it.
     In these cases, message will be sent from system.self.
     
     @rhs {array} A response array.  Can contain up to 3 fields.
     rhs[0] is a function to execute when receive a response.  rhs[1]
     is a number representing the number of seconds to wait before
     stopping listening for a response.  rhs[2] is a function that
     gets executed when we have not received a response within rhs[1]
     seconds.

     @return {ClearObject} @see return type for
     std.messaging.sendMessage.

     
     or

     
     @lhs {MessageReceiverSender} An object containing a
     MessageReceiverPair as well as holding

     @rhs (see above rhs)

     @return {ClearObject} Same as above.  @see return type for
     std.messaging.sendMessage.
     
     */
    std.messaging.sendSyntax = function (lhs, rhs)
    {
        if (checkIsMessageReceiverPair(lhs) && checkIsArray(rhs))
            return callSendMessageReceiverPair(lhs,rhs);
        else if (checkIsMessageReceiverSender(lhs) && checkIsArray(rhs))
            return callSendMessageReceiverSender(lhs,rhs);
        else if ((typeof(lhs) == 'object') && (checkIsVisible(rhs)))
            return new MessageReceiverPair(lhs,rhs);
        //lkjs;
        
        throw 'Error in sender syntax.  Require either that: 1) lhs must be senderReceiverPair and rhs must contain message handling code; or 2) lhs must be object and rhs must be visible.  Aborting.';
    };

    
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

     @return {ClearObject} Returns an object whose methods can be used to call
     "clear", which aborts listening for the response to the message.
     
     Registers the key generated from recString,senderString,seqNo with value of
     a two-long array containing handles to the respHandler and onNoRespTimer.
     */
    function addOpenHandler(recString,senderString, seqNo,respHandler,onNoRespTimer)
    {
        var key = generateKey(recString,senderString,seqNo);
        openHandlers[key] = [respHandler,onNoRespTimer];

        return new ClearObject(key);
    }

    /**
     Calling send message returns this object. This object can be used
     to cancel listening for responses to the message that was sent
     out.
     */
    function ClearObject(key)
    {
        var hasCanceled = false;
        this.key = key;
        this.clear = function()
        {
            if (hasCanceled)
                throw 'Error.  Cannot clear a message stream that was already cancelled.';
            cancelOpenHandler(key);
        };
        
        this.isCleared = function()
        {
            return hasCanceled;
        };
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
        //clear timeout handler if it isn't null
        if (openHandlers[key][1] != null)
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

     @return {ClearObject} Returns an object whose methods can be used to call
     "clear", which aborts listening for the response to the message.

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
        var onNoRespTimeout = null;
        if (timeToWait != null)
            onNoRespTimeout = system.timeout(timeToWait,wrapOnNoResp);

        return addOpenHandler(recString,senderString,seqNo,respHandler,onNoRespTimeout);
        
    };
    
}
)();

