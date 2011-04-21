//run this on one presence, and run testSendMessageVisible.em on
//another.  This will only print messages that have a field 'm'
//that has value 'random' and that has a field 'msg' that has a
//value 'msg'.


test_msg_handler <- [{"m":"random":}, {"msg":"msg":}];

function test_msg_handler(msg,sender)
{
    system.print("\n\n");
    system.print("Got into msg handler");
    system.print("\n\n");
}



