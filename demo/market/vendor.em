system.import("std/library.em");

/* This is a Book Vendor */

/* We will fill this in sometime */
var MARKET_CHANNEL = undefined;

/* This is my adv. ticker */
var Banner = "Books";

var buy_proto = "get_books";


/* fill in the banner object to send to the market */
var bannerObj = undefined;
function createBannerObject()
{

  var f1 = function(init_arg_1, init_arg_2)
                          {  
                            
                             //system.print("\n\nExecuting the init_protocol\n\n");
                             //system.print("init_arg_1 = " + init_arg_1 + "init-arg_2 = " + init_arg_2);
                              
                             init_arg_1 <- new util.Pattern("seq_no", "2") <- init_arg_2; 
                             var ack = new Object(); 
                             ack.name = "ack";
                             ack.seq_no = "1";
                             print("Sending the ack to " + init_arg_2);
                             ack -> init_arg_2;
                             
                             //system.print("\n\ndone executing the init_protocol\n\n");
                          };

  bannerObj = new Object();
  bannerObj.name = "advertisement";
  bannerObj.banner = "Books";
  bannerObj.vendor = system.Self;
  bannerObj.seq_no = 1;
  bannerObj.init_proto =  f1;  
  
  bannerObj.init_arg_1 = function(msg, sender){ 
                          //system.print("\n\nTrying opening the popup window\n\n");
                          var simulator = system.presences[0].runSimulation("ogregraphics");
                          //var dialog_box = simulator.invoke("getChatWindow");
                          var dialog_box = simulator.invoke("getWindow", "chat_terminal", "chat/prompt.html");
                        }

  bannerObj.init_arg_2 = system.Self;

}

//var bannerObj = {"name":"advertisement", "banner":"Books", "init_proto":};


var books_array = ["Introduction to Networking", "Harry Potter - 3"];



function seq_handler(msg, sender){
   print("\n\nGot an ack message\n\n");
   if(msg.seq_no == "1")
   {
     print("Vendor: Got a seq no 1");
     print("Vendor: Sending seq no 2");

     var seq2 = new Object();
     seq2.seq_no = "2";
     seq2 -> sender;
   }
}


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

    handleBookRequest <- [ new util.Pattern("name", "get_books"), new util.Pattern("replyFunction")];
    bannerObj -> MARKET_CHANNEL;
    seq_handler <- new util.Pattern("name", "ack");
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
  var p = new util.Pattern("protocol", "Market");
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

      system.presences[0].onProxAdded(proxAddedCallback);

      createBannerObject();
    }
});

system.onPresenceDisconnected( function() {
    system.print("startupCamera disconnected");
});



