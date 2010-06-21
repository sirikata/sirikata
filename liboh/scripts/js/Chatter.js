// The chatter object


print = function(args)
{
  system.print("Chatter ( " + system.Self + " ): " + args);
}


print("Evaluating Bulletin Board");

bulletinBoardPattern = system.Pattern("BulletinBoard");


bulletinBoard = "";

function getBulletinBoardCallback(msg, sender)
{
  bulletinBoard = msg.sender;

  print("Bulletin Board is " + sender);
}

system.registerHandler(bulletinBoardPattern, null,getBulletinBoardCallback, null);
