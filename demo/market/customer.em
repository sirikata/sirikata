system.import("std/library.em");


/* So, in our simulations, we will just register with the market saying anything that match the regexp *Book*, please give the id to me */

/* We will fill this in sometime */
var MARKET_CHANNEL = undefined;

/* Our pattern of interest is Books. We could also do regex here */
var patternOfInterest = "Books";

/* create a subscription message */
var subsObj = {"name":"subscribe", "pattern":patternOfInterest};

var simulator = undefined;


/* Somehow we need to know about market..this is the handler..we register is later down the code */
function handleMarket(msg, sender)
{
  if(!MARKET_CHANNEL)
  {
    MARKET_CHANNEL = sender;    
    print("\n\nGot a new market\n\n");

    handleMarketReply <- [new util.Pattern("vendor"), new util.Pattern("banner"), new util.Pattern("init_proto")] <- MARKET_CHANNEL;
    //send the subscription

    subsObj -> MARKET_CHANNEL;
    print("\n\n Subscription for books sent to market \n\n");
  }
}

/* This is the callback for all the new visibles that I get from Pinto 
  Send to each of them a test message to see if they can respond to it. 
  Register the handler for test message
*/
function proxAddedCallback(new_addr_obj)
{
  
  var test_msg = new Object();
  test_msg.name = "get_protocol";
  
  //also register a callback for market. the msg should have a "protocol" field with value "Market"
  var p = new util.Pattern("protocol", "Market");
  
  handleMarket <- p <- new_addr_obj;
  print("\n\nRegistered a pattern\n\n");
  test_msg -> new_addr_obj;
}


/* This is the handler for a market. Once you subscribe, market will send you updates about vendors matching your interest
   Execute the init_protocol of the vendors so as to show interest in them. 
   Execute this init_protocol inside a new context, giving access to only required args. 
*/

function handleMarketReply(msg, sender)
{
  print("\n\n Got a reply from the market . The vendor is " + msg.vendor + "\n\n");
  
  // Creating a new context
  //var newContext = system.create_context(system.presences[0], msg.vendor, true, true, true);
  
  msg.init_proto(msg.init_arg_1, msg.vendor, system.presences[0]);

  //newContext.execute(msg.init_proto, msg.init_arg_1, msg.init_arg_2, system.presences[0]); 
}


// Set up the camera
system.onPresenceConnected( function(pres) {
                                
    system.print("startupCamera connected " + pres);
    system.print(system.presences.length);
    if (system.presences.length == 1)
    {
      system.presences[0].onProxAdded(proxAddedCallback);
    }
});

system.onPresenceDisconnected( function() {
    system.print("startupCamera disconnected");
});




