/* This is the Market Script  */

/* vendors must send their banner to the market to attract customers */
/*Customers must send subscription to get the vendors of their interest */


/* Keep a dictionary of customers */
var customer = new Object();

/* Keep a dictionary of the vendors */
var vendor = new Object();

/* Handle an advertisement message */
function advReactor(msg, sender)
{
  vendor[sender].banner = msg.banner;    

  for( s in customer)
  {
    if(matches(customer[s].pattern, vendor[sender].banner))
    {
      var reply_obj = new Object();
      reply_obj.vendor = sender;
      reply_obj.banner = vendor[sender].banner;
      //send this reply to this customer
      reply_obj -> s;
    }
  }
}



/* Handle a subscription message */
function subscriptionReactor(msg, sender)
{
  /* Note down the pattern of interest of this customer */
	/* You identify the customer with the sender id */
  customer[sender].pattern = msg.pattern; 
  
  //Check if there exist a vendor that satisfies this customer's pattern

  for(s in vendor)
  {
    if(matches(customer[sender].pattern, vendor[s].banner))
    {
      var reply_obj = new Object();
      reply_obj.vendor = s;
      reply_obj.banner = vendor[s].banner;
      // send this reply to this customer
      reply_obj -> sender;
    }
  }
}


// THis will send reply to "get_protocol" message from customers or vendors
function handleProtocolMessage(msg, sender)
{
  var msg_obj = new Object();
  msg_obj.protocol = "Market";
  msg_obj -> sender;
}



/* React to the advertisements */
advReactor <- [new system.Pattern("name", "advertisement"), new system.Pattern("banner")];

/* React to the subscriptions */
subscriptionReactor <- [new system.Pattern("name", "subscribe"), new system.Pattern("pattern")];

handleProtocolMessage <- new system.Pattern("name", "get_protocol");



