system.import("std/library.em");

/* This is a Book Vendor */

/* We will fill this in sometime */
var MARKET_CHANNEL = undefined;

/* This is my adv. ticker */
var Banner = "Books";

var buy_proto = "get_books";


/* fill in the banner object to send to the market */
var bannerObj = undefined;

var my_books = ["Introduction to Computer Networks", "Introduction to Operating Systems", "Wireless Networks"];
function createBannerObject()
{


  var f1 = function(init_arg_1, init_arg_2, pres)
                          {
                             var handler = function(msg, sender)
                             {
                                init_arg_1(msg, sender, pres);
                             }

                             handler <- new util.Pattern("seq_no", "2") <- init_arg_2;
                             var ack = new Object(); 
                             ack.name = "ack";
                             ack.seq_no = "1";
                             //print("Sending the ack to " + init_arg_2);
                             ack -> init_arg_2;
                             
                          };

  bannerObj = new Object();
  bannerObj.name = "advertisement";
  bannerObj.banner = "Books";
  bannerObj.vendor = system.Self;
  bannerObj.seq_no = 1;
  bannerObj.init_proto =  f1;  
  var f2 = function(msg, sender, pres)
                         { 
                           
                           var simulator = pres.runSimulation("ogregraphics");
                          
                           var code = "<html><head><style type=\"text/css\">.command {border-width: 1px 1px 1px 1px;border-color: black;border-style: solid;padding: 0px;}</style><script>function onYes(){var arg_map = ['ExecScript', 'Command', 'Yes'];chrome.send('event', arg_map);}function onNo(){var arg_map = ['ExecScript', 'Command', 'No'];chrome.send('event', arg_map);}</script></head><body>A Vendor wants to send you the list of books you may be interested in. Do you wish to look at it?<br><button type=\"button\" onclick=\"onYes()\">Yes</button><br><button type=\"button\" onclick=\"onNo()\">No</button></body></html>";

                            
                           var my_books = ["Introduction to Computer Networks", "Introduction to Operating Systems", "Wireless Networks"];
                          
                           var book_form_code = "<html><head><script>function onBuy(book){ document.write('<p>Hello</p>'); var arg_map = ['ExecScript', 'Command', book]; chrome.send('event', arg_map); }</script></head><body>Introduction to Computer Networks <button type=\"button\" onclick=\"onBuy(book1)\">Buy</button><br>Introduction to Operating Systems<button type=\"button\">Buy</button></body></html>";

                          
                           var dialog_box = simulator.invoke("createWindowHTML", "chat_terminal", code);
                             
                           var event_handler = function(arg)
                           {
                             
                             if(arg == "Yes")
                             {
                               
                               var book_form = simulator.invoke("createWindowHTML", "book_form", book_form_code);
                               book_form.invoke("bind", "event", function(arg2){ /*system.print("Trying to buy the book " + arg2);*/ } );
                             }
                                                      

                         };
                         
                         dialog_box.invoke("bind", "event",  event_handler);
                        
                       };

  bannerObj.init_arg_1 = f2;
  bannerObj.init_arg_2 = system.Self;

}



var books_array = ["Introduction to Networking", "Harry Potter - 3"];



// Handle messages based on their sequence numbers
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
