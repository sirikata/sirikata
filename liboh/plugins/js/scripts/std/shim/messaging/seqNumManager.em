
(function()
{
    var NULL_STREAM_ID = -1;
    var NULL_SEQ_NUM   = -1;
    
    std.messaging.seqNumManager = {  };
    
    //FIXME: May want to build in some form of garbage collection for
    //the seqNumMap;

    //This keeps track of what sequence number we're on for each
    //sender, receiver pair.  Its keys are the sender.toString() +
    //receiver.toString() + stream id.  Its values are the sequence number that
    //you should send on your next message.
    var seqNumMap = { };

    /**
     @param {presence} me
     @param {visible} them
     @param {number} streamID
     */
    function generateKey(me,them,streamID)
    {
        return me.toString() + '--' + them.toString() + '***' + streamID.toString();
    }
    
    /**
     @param me is the local presence from which I can send and receive
     messages.
     @param them is the external presence (visible) that is supposed
     to receive the message from me, and send messages to me.

     @return Returns a sequence number that we have not yet used for
     this connection.
     */
    std.messaging.seqNumManager.getSeqNumber = function (me, them,streamID)
    {
        if (streamID == NULL_STREAM_ID)
            return NULL_SEQ_NUM;
        
        var key = generateKey(me,them,streamID);

        if (!(key in seqNumMap))
            seqNumMap[key] = 0;

        return seqNumMap[key];
    };


    
    
    /**
     Should be called whenever receive a response from them to me on
     stream streamID and sequence number seqReceived.
     @return returns latest seqNumber to send.
     */
    function updateSeqNumberOnReceivedMessageInternal (me,them,streamID, seqReceived)
    {
        if (streamID == NULL_STREAM_ID)
            return NULL_SEQ_NUM;

        
        var key = generateKey(me,them,streamID);
        seqNumMap[key] = seqReceived + 1;
        return seqNumMap[key];
    };

    std.messaging.seqNumManager.updateSeqNumberOnReceivedMessage = function(me,them,msg)
    {
        var seqNo = -1;
        if (typeof(msg.seqNo) == 'number')
            seqNo = msg.seqNo;

        var streamID = -1;
        if (typeof(msg.streamID) =='number')
            streamID = msg.streamID;

        return updateSeqNumberOnReceivedMessageInternal(me,them,streamID,seqNo);
        
    };

    
    
})();
