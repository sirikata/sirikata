/**
 Tests whether two presences on the same entity can exchange a bunch
 of messages using messaging syntax, etc.
 
 Scene.db
   * Ent 1: Anything

 
 Duration: 8s

 Tests: message syntax, onPresenceConnected, createPresence,
 system.presences, serialization and deserialization (for messages),
 msg.makeReply.
 
 */

system.require('helperLibs/util.em');
mTest = new UnitTest('messagingTest');
numPresConnected = 0;

singleMessageTestsCompleted = false;
streamMessageTestsCompleted = false;
noReplyTestCompleted        = false;

system.onPresenceConnected(runTests);


function runTests()
{
    ++numPresConnected;
    if (numPresConnected != 2)
    {
        system.createPresence({});
        return;
    }

    pres1 = system.presences[0];
    pres2 = system.presences[1];

    runSingleMessageTests();
    runStreamMessageTests();
    runNoReplyTest();

    system.timeout(5, finalCleanup);
}



//callback for when receive a message that matches the pattern:
//{'singleMessageTest'::}.  These messages were sent from
//runSingleMessageTests.
function handleSingleMessage(selfArray, senderArray,msg,sender)
{
    if (!('singleMessageTest' in msg) ||
        (msg['singleMessageTest'] !== 1))
    {
        mTest.fail('Failure in handleSingleMessage callback.  '+
                   'Message processed did not have correct fields:');
        system.prettyprint(msg);
    }
    selfArray.push(system.self);
    senderArray.push(sender);

    if (selfArray.length > 2)
    {
        mTest.fail('Failure in handleSingleMessage callback.  '+
                   'Received more than two callbacks for: ');
        system.prettyprint(selfArray);
    }
}




function finishHandleSingleMessage(selfArray,senderArray)
{

    //did we receive two and exactly two messages?
    if (selfArray.length != 2)
    {
        mTest.fail('Failure in singleMessageTests.  Sent two singleMessages.  '+
                   'Should have received two back.  Instead, got ' +
                   selfArray.length.toString());

        singleMessageTestsCompleted = true;
        return;
    }

    //now actually check to ensure that got correct sender/self combinations.
    if (selfArray[0].toString() === selfArray[1].toString())
    {
        mTest.fail('Failure in singleMessageTests.  Two singleMessages ' +
                   'arrived to the same self.');
    }
    if (senderArray[0].toString() === senderArray[1].toString())
    {
        mTest.fail('Failure in singleMessageTests.  Two singleMessages ' +
                   'arrived from the same sender.');
    }

    if (selfArray[0].toString() == senderArray[0].toString())
    {
        mTest.fail('Failure in singleMessageTests.  Sender and self same ' +
                   'for first received message.');
    }
    if (selfArray[1].toString() == senderArray[1].toString())
    {
        mTest.fail('Failure in singleMessageTests.  Sender and self same ' +
                   'for second received message.');
    }

    system.print('\n\nsingleMessageTest complete.\n\n');
    singleMessageTestsCompleted = true;
}


//send a simple message from pres1 to pres2 and send the same message
//from pres2 to pres1.  Ensure that both get to the other side and
//that selfs and senders are correct.  Also tests message matching.
function runSingleMessageTests()
{
    var selfSeenInMsgHandler   = [];
    var senderSeenInMsgHandler = [];
    pres1 # {'singleMessageTest': 1 } >> pres2 >> [];
    pres2 # {'singleMessageTest': 1 } >> pres1 >> [];
    
    std.core.bind(handleSingleMessage,
                  undefined,
                  selfSeenInMsgHandler,
                  senderSeenInMsgHandler) << {'singleMessageTest':: };

    //messages have to transfer within 5 seconds.
    system.timeout(3,
        std.core.bind(finishHandleSingleMessage,
                      undefined,
                      selfSeenInMsgHandler,
                      senderSeenInMsgHandler));
}


//tests makeReply on streams of messages between pres1 and pres2.
function runStreamMessageTests()
{
    MAX_NUM_STREAMED_MESSAGES = 20;
    var msgList = [];
    pres1 # {'streamMessageTest':1, 'index': 0} >>
            pres2 >>
              [std.core.bind(streamMessageTestHandler, undefined, msgList),5];

    std.core.bind(streamMessageTestHandler,undefined,msgList)
        << {'streamMessageTest':1:};
    
    system.timeout(3, std.core.bind(finishStreamMessageTests,undefined,msgList));
}

function streamMessageTestHandler(msgList,msg,sender)
{
    msgList.push(msg);
    if (msgList.length < MAX_NUM_STREAMED_MESSAGES )
    {
        msg.makeReply({'index': msgList.length})
            >> [std.core.bind(streamMessageTestHandler, undefined, msgList),5];
    }
}

function finishStreamMessageTests(msgList)
{
    if (msgList.length != MAX_NUM_STREAMED_MESSAGES)
    {
        mTest.fail('Failure in streamMessageTest.  Expected ' +
                   MAX_NUM_STREAMED_MESSAGES.toString()       +
                   ' received ' + msgList.length.toString()   +
                   ' messages.');
    }

    for (var s =0; s < msgList.length; ++s)
    {
        if (!('index' in msgList[s]))
        {
            mTest.fail('Failure in streamMessageTest.  ' +
                       'Messages should have index fields.');                
        }
        else
        {
            if (msgList[s].index !== s)
            {
                mTest.fail('Failure in streamMessageTest.  '    +
                           'Message\'s index field should be: ' + s.toString());                
            }
        }
            
    }

    streamMessageTestsCompleted = true;

    system.print('\n\nstreamMessageTest complete.\n\n');
}


//tests to ensure that the no reply function actually fires when don't
//get a response to a message.
function runNoReplyTest()
{
    var gotReply   = false;
    var gotNoReply = false;
    {'testNoReply':1 } >> pres2 >> [function()
                                    {
                                        gotReply = true;
                                        mTest.fail('Failure in testNoReply.  '+
                                                   'Should not have received '+
                                                   'reply to message.');
                                    },
                                    2,
                                    function()
                                    {
                                        gotNoReply = true;
                                    }];

    system.timeout(3,
                  function()
                  {
                      if (gotReply)
                          mTest.fail('Failure in testNoReply.  Got a reply '+
                                     'for a message when I should not have '+
                                     'gotten a reply.');
                      if (!gotNoReply)
                          mTest.fail('Failure in testNoReply.  No reply ' +
                                     'callback was not called when did '  +
                                     'not receive reply.');

                      system.print('\n\ntestNoReply complete\n\n');
                      noReplyTestCompleted = true;
                  });
}


function finalCleanup()
{
    if (!singleMessageTestsCompleted)
        mTest.fail('Did not complete singleMessageTest');
    if (!streamMessageTestsCompleted)
        mTest.fail('Did not complete streamMessageTest');
    if (!noReplyTestCompleted)
        mTest.fail('Did not complete noReplyTest');

    
    
    if ((singleMessageTestsCompleted) && (streamMessageTestsCompleted) &&
        (noReplyTestCompleted) && (!mTest.hasFailed))
        mTest.success('Messaging test success.');
    
    system.killEntity();
}
