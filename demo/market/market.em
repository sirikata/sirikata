 system.import("std/library.em");
 
 /*This is the Market Script  */

/* vendors must send their banner to the market to attract customers */
/*Customers must send subscription to get the vendors of their interest */


/* Keep a dictionary of customers */
var customer = new Object();

/* Keep a dictionary of the vendors */
var vendor = new Object();

/* Handle an advertisement message */
function advReactor(msg, sender)
{
  vendor[sender] = msg;
   
  print("\n\n Got a vendor banner = " + vendor[sender].banner + " : " + msg.banner + "\n\n");
  for( s in customer)
  {
    if(matches(customer[s].pattern, vendor[sender].banner))
    {
      var reply_obj = new Object();
      reply_obj.vendor = sender;
      reply_obj.banner = vendor[sender].banner;
      reply_obj.init_proto = vendor[sender].init_proto;
      //send this reply to this customer
      reply_obj -> s;
    }
  }

}



/* Handle a subscription message */
function subscriptionReactor(msg, sender)
{
  print("\n\n Customer is subscribing with the market\n\n");
  /* Note down the pattern of interest of this customer */
	/* You identify the customer with the sender id */
	customer[sender.toString()] = new Object();
        print("\n\n Herer\n\n");
  customer[sender.toString()].pattern = msg.pattern; 
  
  //Check if there exist a vendor that satisfies this customer's pattern

  for(s in vendor)
  {
	  print("Trying to match cutomer.pattern = " + customer[sender].pattern + " against vendor.banner = " + vendor[s].banner);
    if(matches(customer[sender].pattern, vendor[s].banner))
    {
		   
		  print("\n\n Vendor match found for this new customer \n\n");
      var reply_obj = vendor[s];
      reply_obj -> sender;
      print("\n\n Sent the vendor infor for " + s + "to the customer\n\n");
    }
  }

	print("\n\n Customer subscribed with the market\n\n");
}


// THis will send reply to "get_protocol" message from customers or vendors
function handleProtocolMessage(msg, sender)
{
  var msg_obj = new Object();
  msg_obj.protocol = "Market";
  msg_obj -> sender;
}



/* React to the advertisements */
advReactor <- [new util.Pattern("name", "advertisement"), new util.Pattern("banner"), new util.Pattern("init_proto")];

/* React to the subscriptions */
subscriptionReactor <- [new util.Pattern("name", "subscribe"), new util.Pattern("pattern")];

handleProtocolMessage <- new util.Pattern("name", "get_protocol");


