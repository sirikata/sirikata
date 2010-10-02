/*
  This is a Market Place Script. 
  This is an entity which is actually with no mesh ( invisible )
		but has presence in the world. ( Might have location property to exploit the distance analogy )

		It has only one presence for simplicity. This presence is it's address in the world. It's presence
		is already published to the vendors and customers as soon as they join in the market place.

		This entity functions as a multicast entity. It takes the messages that it gets and forwards it to
		the logged in vendors and customers. Moreover, it does the content based filtering as required by the 
		customers. So, the trick is that the filtering of the packet is not being done at the customer end, but
		at the market place ( could be a good optimization too ).
*/


/* Every vendor is an avatar and is allowed to
   advertise on this channel
	*/




var MARKET_CHANNEL = system.presences[0]; // or we have the keyword self for this




var customer_list = new Object();

// handler for the proximity queries
// add the presences in the proximity query result to the customer list
function proximity_handler(msg, sender)
{
  customer_list.set(msg.presence_list);
}

//handler for the incoming subscripting message

function subscription_handler(msg, sender)
{
  customer_list[sender].pattern = msg.pattern; 
}

function l_matches(pat, str)
{
  var regex = new RegExp(pat, "i");
  regex.compile(regex);
  return regex.test(str);  
}



// handler for incoming advertisement message
function multicast(msg, sender)
{
  for(customer in customer_list)
  {
		  // if the customer pattern matches the msg pattern
				// then send the msg to the customer
		  if( l_matches(customer.pattern, msg.pattern)) 
		    {
		       msg -> customer;
		    }
  }
}


// register proximity queries and add people as customers.

system.registerProximity(presences[0]);

// register the handler for multicast from the vendors
multicast <- new Pattern("advertisement");

//register the handler for the subscripting messages from the customers

patterns = new Array();
patterns[0] = new Pattern("name", "subscribe");
patterns[1] = new Pattern("pattern");

subscription_handler <- patterns; 
