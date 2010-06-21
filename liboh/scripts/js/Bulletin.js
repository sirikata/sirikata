// The bulletin board .

print = function(args)
{
  system.print("Bulletin Board: " + args);
}

print("Evaluating Bulletin Board");

// returns true if x is contained in array A
function contains(x, A)
{
  for(i in A)
  {
    if(x == A[i])
	{
	  return i;
	}
  }
  return -1;
}

//Create a hash table of the registered users
// THe key is the username and the value is hte
// list of statements of the user

registeredUsers = new Array();
registeredStmts = new Array();

addPattern = system.Pattern('add');
readPattern = system.Pattern('read');


sendRegistrationMessage = function(dest)
{
  var msg = new Object();
  msg.result = "OK";
  sender.sendMessage(msg);  
}

// callback when the user sends an add request
addCallback = function(obj, sender){
  print("Got a message from " + sender.toString());  	
  // check if the user is known to us
  var x = contains(sender, registeredUsers);
  if( x != -1)
  {
    //user is known to us
	// add the messge field
    registeredStmts[x] += obj.add;
   
  }
  else
  {
    registeredUsers.push(sender);
	registeredStmts[registeredUsers.length - 1] = "";
  }

  // register a callback fro a read messaage
  // read will be called back only when we have a message from the registered users
  system.registerHandler(readPattern, null, readCallback, sender);

  sendRegistrationMessage();
}




readCallback = function(obj, sender){
  
  print("Got a message from " + sender.toString());  	

  // reader is definitely known to us

  var msg = new Object();
  // a dictionary could have been helpful
  // may be as a library
  var xx = contains(sender, registeredUsers);
  msg.val = registeredStmts[x];

  sender.sendMessage(msg);


}

broadCastCallback = function()
{
  print("BroadCasting myself ");
  var bMsg = new Object();
  bMsg.BulletinBoard = true;
  bMsg.sender = system.Self;

  system.__broadcast(bMsg);
  system.timeout(5, null, broadCastCallback);
}

system.timeout(5, null, broadCastCallback);
system.registerHandler(addPattern, null, addCallback, null);



// broadCast that you are bulletiing board




print("Done with evaluation of the Bulletin board");
