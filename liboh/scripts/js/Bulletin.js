// The bulletin board .

print = function(args)
{
  system.print("\n\n\nBulletin Board: " + args + "\n\n\n");
}

print("Evaluating Bulletin Board");

registeredUsers = new Object();

addPattern = new system.Pattern('add');
readPattern = new system.Pattern('read');


sendAddConfirmationMessage = function(dest)
{
  var msg = new Object();
  msg.result = "OK";
  dest.sendMessage(msg);  

  print("Sent add confirmation to " + dest);
}

// callback when the user sends an add request
function addCallback(obj, sender){
  print("Got a ADD message from " + sender.toString());  	
  // check if the user is known to us

  var added = false;
  if(registeredUsers[sender])
  {

    print("User already exists");
    added = true;  
    registeredUsers[sender].statements += obj.add;
  }
  else
  {
    registeredUsers[sender] = new Object();
	if(obj.add)
	{
	  registeredUsers[sender].statements = obj.add;
	}
	else
	{
	  
	  registeredUsers[sender].statements = "";
	}

    print("User is new. Adding a read callback for the user "+sender);
    system.registerHandler(readPattern, null, readCallback, sender);

  }
  
  sendAddConfirmationMessage(sender);
}

function readCallback(obj, sender){
  
  print("Got a READ message from " + sender.toString());  	

  // reader is definitely known to us

  var msg = new Object();
  
  if(registeredUsers[sender])
  {
    print("The user " + sender + "exists.");

    msg.value = registeredUsers[sender].statements;
    sender.sendMessage(msg);
	
	print("Sent the read reply ( " + msg.value + ") to "+ sender);
  }

}

function broadCastCallback()
{
  print("BroadCasting myself ");
  var bMsg = new Object();
  bMsg.BulletinBoard = true;
  bMsg.sender = system.Self;

  system.__broadcast(bMsg);
  system.timeout(30, null, broadCastCallback);
}

system.timeout(30, null, broadCastCallback);
system.registerHandler(addPattern, null, addCallback, null);



// broadCast that you are bulletiing board


print("Done with evaluation of the Bulletin board");
