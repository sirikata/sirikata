system.import('std/graphics/default.em');

/* Make Customer send out a touch event to the Book entity */

/* question is how do you know the uuid of the Book entity */
/* In a real world you just know that the book is in your front, so just touching it generates an event with appr. params */

/* So, in our simulations, we will just register with the market saying anything that match the regexp *Book*, please give the id to me */

/* We will fill this in sometime */
var MARKET_CHANNEL = undefined;

/* Our pattern of interest is Books */
var patternOfInterest = new util.Pattern("pattern", "/*Books*/");

/* create a subscription message */
var subsObj = {"name":"subscribe", "pattern":patternOfInterest};

var simulator = undefined;

function handleMarket(msg, sender)
{
  if(!MARKET_CHANNEL)
  {
    MARKET_CHANNEL = sender;    
    print("\n\nGot a new market\n\n");

    handleMarketReply << [new util.Pattern("vendor"), new util.Pattern("banner")] << MARKET_CHANNEL;
  }
}

// arg here is an addressable object
function proxAddedCallback(new_addr_obj)
{
  print("\n\n\n\n\n\n\n\n  Got a proxAdded Callback \n\n\n\n\n\n\n");

  var test_msg = new Object();
  test_msg.name = "get_protocol";
  
  //also register a callback
  var p = new util.Pattern("protocol", "Market");
  handleMarket << p << new_addr_obj;
  test_msg -> new_addr_obj;
}

// This is the callback for the reply from the market

function handleMarketReply(msg, sender)
{
  print("\n\n\n\n\\n\n\n\n Got a reply from the market \n\n\n\n\n\n\n\n\n"); 
  var vendor = msg.vendor;
	var banner = msg.banner;

  print("Got a vendor " + vendor);
}


// Set up the camera

system.onPresenceConnected( function(pres) {
                                
    system.print("startupCamera connected " + pres);
    system.print(system.presences.length);
    if (system.presences.length == 1)
    {
        simulator = new std.graphics.DefaultGraphics(pres, 'ogregraphics');
      system.presences[0].onProxAdded(proxAddedCallback);
    }
});

system.onPresenceDisconnected( function() {
    system.print("startupCamera disconnected");
});
