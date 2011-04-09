//run this on one presence, and run testSendMessageVisible.em on another.

function test_msg_handler(msg,sender)
{
    system.print("\n\n");
    system.print("Got into msg handler");
    system.print("\n\n");
}


//var matchPattern = new util.Pattern("msg");
//test_msg_handler <- {"name":"test":};

//var matchPattern = {"m":"random" :};

// toBroadcast.msg = "msg";
// toBroadcast.m = "random"

// system.registerHandler(test_msg_handler,
//                        null,
//                        {"m":"random" :},
//                        null);

test_msg_handler <- {"m":"random":};
