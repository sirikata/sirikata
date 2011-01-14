
/* Make Customer send out a touch event to the Book entity */

/* question is how do you know the uuid of the Book entity */
/* In a real world you just know that the book is in your front, so just touching it generates an event with appr. params */

/* So, in our simulations, we will just register with the market saying anything that match the regexp *Book*, please give the id to me */

/* We will fill this in sometime */
var MARKET_CHANNEL = undefined;

/* Our pattern of interest is Books */
//var patternOfInterest = new system.Pattern("pattern", "/*Books*/");
var patternOfInterest = "Books";

/* create a subscription message */
var subsObj = {"name":"subscribe", "pattern":patternOfInterest};

var simulator = undefined;

function handleMarket(msg, sender)
{
  if(!MARKET_CHANNEL)
  {
    MARKET_CHANNEL = sender;    
    print("\n\nGot a new market\n\n");

    handleMarketReply <- [new system.Pattern("vendor"), new system.Pattern("banner"), new system.Pattern("init_proto")] <- MARKET_CHANNEL;
    //send the subscription

    subsObj -> MARKET_CHANNEL;
    print("\n\n Subscription for books sent to market \n\n");
  }
}

// arg here is an addressable object
function proxAddedCallback(new_addr_obj)
{
  print("\n\n\n\n\n\n\n\n  Got a proxAdded Callback \n\n\n\n\n\n\n");
  print("The new addressable is " + new_addr_obj);
  
  var test_msg = new Object();
  test_msg.name = "get_protocol";
  
  //also register a callback
  var p = new system.Pattern("protocol", "Market");
  
  print("\n\nRegistering  a pattern\n\n");
  handleMarket <- p <- new_addr_obj;
  print("\n\nRegistered a pattern\n\n");
  test_msg -> new_addr_obj;
  print("\n\nOut of the prox added callback\n\n");
}

function handleBookList(msg, sender)
{
  print("\n\nGot a book List\n\n");    
}

// This is the callback for the reply from the market

function handleMarketReply(msg, sender)
{
  print("\n\n Got a reply from the market . The vendor is " + vendor + "\n\n");
  var vendor = msg.vendor;
  var banner = msg.banner;
  var init_proto = msg.init_proto;

  var req_obj = new Object();
  req_obj.name = init_proto;
  
  req_obj.replyFunction = function(list_of_books, wrapped_reply)
                          {
                            wrapped_reply = new Object();
                            wrapped_reply.reply = list_of_books;
                            wrapped_reply.seq_no = 1;
                          }

  
  //handleBookList <- [new system.Pattern("reply"), new system.Pattern("seq_no", 1)] <- vendor;
  req_obj -> vendor;
  print("Sent book list request to the vendor " + vendor);  

}


// Set up the camera

system.onPresenceConnected( function(pres) {
                                
    system.print("startupCamera connected " + pres);
    system.print(system.presences.length);
    if (system.presences.length == 1)
    {
      //simulator = pres.runSimulation("ogregraphics");
      system.presences[0].onProxAdded(proxAddedCallback);
    }
});

system.onPresenceDisconnected( function() {
    system.print("startupCamera disconnected");
});


