//run this on one presence, and run testSendMessageVisible.em on
//another.  This will only print messages that have a field 'm'
//that has value 'random' and that has a field 'msg' that has a
//value 'msg'.


test_msg_handler <- [{"m":"random":}, {"msg":"msg":}];

//if want to match messages with field m, but any value in that field.
//test_msg_handler <- [{"m"::}];
//if want to match all messages:  ***note, will fix so that has cleaner syntax***.
//test_msg_handler <- [new util.Pattern()];

numReceived = 0;
function test_msg_handler(msg,sender)
{
    system.print("\n\n");
    system.print("Got into msg handler");
    system.print(numReceived);
    ++numReceived;
    system.print("\n\n");
}



