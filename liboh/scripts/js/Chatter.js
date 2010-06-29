// The chatter object


print = function(args)
{
  system.print("\n\n\nChatter ( " + system.Self + " ): " + args + "\n\n\n");
}


print("Evaluating Chatter");

bulletinBoardPattern = new system.Pattern("BulletinBoard");
addConfPattern = new system.Pattern("result");
readResultPattern = new system.Pattern("value");

bulletinBoard = "";
added = false;

function sendAddRequest()
{
  var reg = new Object();
  reg.add = "Hello";
  bulletinBoard.sendMessage(reg);

  print("Sent an add request to the Bulletin Board");
}

function addConfCallback(obj, sender)
{
  if(obj.result != "OK")
  {
    sendAddRequest();
	added = false;
  }
  else
  {
    added = true;
	print("Registered Successfully with the bulletinBoard");
  }


}

function sendReadMessage()
{
  var msg = new Object();
  msg.read = "";
  bulletinBoard.sendMessage(msg);

  print ( "Sent a read message");
}

function readResultCallback(msg, sender)
{
  var result = msg.value;

  print(" Got the read result as : " + result.toString());
}

function getBulletinBoardCallback(msg, sender)
{
  if(added)
  {
    return;
  }
  bulletinBoard = sender;

  print("Bulletin Board is " + sender);

  // register for a "OK confirmation from the bulletin board

  
  if( !added )
  {

    system.registerHandler(addConfPattern, null, addConfCallback, sender);
    sendAddRequest();
  }	

  // we want to read what we sent
  system.timeout(60, null, sendReadMessage);
  //register rad callback
  system.registerHandler(readResultPattern, null,readResultCallback, bulletinBoard);
}



system.registerHandler(bulletinBoardPattern, null,getBulletinBoardCallback, null);



