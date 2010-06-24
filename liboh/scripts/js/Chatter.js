// The chatter object


print = function(args)
{
  system.print("\n\n\nChatter ( " + system.Self + " ): " + args + "\n\n\n");
}


print("Evaluating Bulletin Board");

bulletinBoardPattern = new system.Pattern("BulletinBoard");


bulletinBoard = "";

function getBulletinBoardCallback(msg, sender)
{
  bulletinBoard = msg.sender;

  print("Bulletin Board is " + sender);
}

system.registerHandler(bulletinBoardPattern, null,getBulletinBoardCallback, null);
