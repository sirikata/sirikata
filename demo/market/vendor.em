/* This is a Book Vendor */

/* We will fill this in sometime */
var MARKET_CHANNEL = undefined;

/* This is my adv. ticker */
var Banner = "Books";

var buy_proto = "get_books";


/* fill in the banner object to send to the market */
var bannerObj = {"name":"advertisement", "banner":"Books", "init_proto":"get_books"};


var books_array = ["Introduction to Networking", "Harry Potter - 3"];

function handleBookRequest(msg, sender)
{
  print("Sending the list of the books available at the store to the Customer : " + sender);  
  var my_reply;
  msg.replyFunction(books_array, my_reply);
  my_reply -> sender;
  return books_array;  
}




function handleMarket(msg, sender)
{
  if(!MARKET_CHANNEL)
  {
    MARKET_CHANNEL = sender;    
    print("\n\nGot a new market\n\n");

    handleBookRequest <- [ new system.Pattern("name", "get_books"), new system.Pattern("replyFunction")];
    bannerObj -> MARKET_CHANNEL;
    print("\n\n Banner Sent to the market\n\n");

  }
}


// arg here is an addressable object
function proxAddedCallback(new_addr_obj)
{
  print("\n\n\n\n Got a new addressable \n\n\n");
  var test_msg = new Object();
  test_msg.name = "get_protocol";
  
  //also register a callback
  var p = new system.Pattern("protocol", "Market");
  handleMarket <- p <- new_addr_obj;
  test_msg -> new_addr_obj;
}

// Set up the camera

system.onPresenceConnected( function(pres) {
                                
    system.print("startupCamera connected " + pres);
    system.print(system.presences.length);
    if (system.presences.length == 1)
    {
      system.print("Printing ..." + system.Self.prototype);
      //simulator = pres.runSimulation("ogregraphics");
      system.presences[0].onProxAdded(proxAddedCallback);
    }
});

system.onPresenceDisconnected( function() {
    system.print("startupCamera disconnected");
});



