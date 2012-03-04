
if (typeof(std.core.SimpleInput) != 'undefined')
    throw new Error('Error in std.core.SimpleInput.  May only import once.');

(function ()
 {
     var knownUsed = [];

     var SIMPLE_INPUT_MOD_NAME = 'SimpleInput';
     var haveInited = false;
     var guiMod = simulator._simulator.addGUITextModule(
         SIMPLE_INPUT_MOD_NAME,
         getSimpleInputGUIText(),
         function()
         {
             haveInited = true;
             guiMod.bind('responseReceived',
                         respReceivedFunc);
             guiMod.bind('htmlSetupComplete',
                        setupNotificationReceived);
         }
     );
     

     function respReceivedFunc(forWhom,response)
     {
         if ((!(forWhom in knownUsed)) ||
             (knownUsed[forWhom] == null))
         {
             throw new Error('\nWarning in simple input: ' +
                             'recevied response for '      +
                             forWhom.toString() + ' when ' +
                             'have no record for it.\n');

         }

         var inputter = knownUsed[forWhom];
         knownUsed[forWhom] = null;
         inputter.cb(response);
     }

     //html sends an event to us, which gets handled by this function.
     //Tells us that the simpleInput gui has been displayed
     function setupNotificationReceived(id)
     {
         if ((!(id in knownUsed)) ||
             (knownUsed[id] ===null))
         {
             throw new Error ('In setupNotificationReceived, had' +
                              'have no record of id to complete ' +
                              'setup for.');
         }

         knownUsed[id].setupComplete =true;
     }
     
     
     /**
      @param {int} type what type of simple input going to provide
      (See std.core.SimpleInput.SELECT_LIST and
      std.core.SimpleInput.ENTER_TEXT as examples
      
      @param {string} message what message gets written at the top of
      the input.

      @param {function} cb Takes a single argument -- the value of
      what was selected.
 
      @param additional (optional)
      
        if ENTER_TEXT is selected, then this should be undefined.
      
        if SELECT_LIST is entered as type, then this should be an
        arrray of tuples.  The first element of each tuple should
        contain the text that gets displayed to the user next to the
        option.  The second element should contain a code that gets
        passed back to cb so that the scripter knows what was
        selected.  Code must be text.

        if NO_INPUT is entered as type, then just display message
        without getting any response back.
      */
     std.core.SimpleInput = function(type, message, cb, additional)
     {
         if (!haveInited)
             throw new Error('Error.  GUI still initializing.  Please wait.');

         this.type       = type;
         this.message    = message;
         this.cb         = cb;
         this.additional = additional;
         this.setupComplete = false;
         
         if ((type == std.core.SimpleInput.SELECT_LIST) &&
             (typeof(this.additional) == 'undefined'))
         {
             throw new Error ('Error constructing SimpleInput.  If given ' +
                              'a list input, should be given additional '  +
                              'labels.');
         }

         this.indexUsed = addToKnownUsed(this);

         var callFunc = '';
         if (type == std.core.SimpleInput.SELECT_LIST)
             callFunc = 'simpleInputSelectList';
         else if (type == std.core.SimpleInput.ENTER_TEXT)
             callFunc = 'simpleInputSelectText';
         else if (type == std.core.SimpleInput.NO_INPUT)
             callFunc = 'simpleInputNoInput';
         else
             throw new Error ('Error: please select a valid type ' +
                              'for SimpleInput');

         guiMod.call(callFunc,message,this.indexUsed,additional);
     };

     
     /*** Types of simple input to get  ***/
     //note: if add to any additional ones of these, then
     //must also add to if-else if statement in constructor
     //of SimpleInput
     std.core.SimpleInput.SELECT_LIST = 0;
     std.core.SimpleInput.ENTER_TEXT  = 1;
     //used just to display a message, not to get any user response
     //from.
     std.core.SimpleInput.NO_INPUT = 2;
     
     std.core.SimpleInput.ready = function()
     {
         return haveInited;
     };


     std.core.SimpleInput.prototype.clear = function()
     {
         
         if ((!haveInited) || (!this.setupComplete))
         {
             //NOTE: this isn't actually safe.  If scripter calls
             //clear before graphics (for all simpleInputs) are
             //inited, will still see
             //the graphics object (briefly), and may be able to
             //respond to its selection in this way.
             //Using this solution for now, because it's easy, and
             //unlikely to actually cause a problem.
             system.timeout(.1,
                 std.core.bind(this.clear,this));
             return;
         }

         guiMod.call('clearSimpleInputWindow',this.indexUsed);
         
         if ((!(this.indexUsed in knownUsed)) ||
             (knownUsed[this.indexUsed] == null))
         {
             throw new Error('\nWarning in simple input clear. ' +
                             this.indexUsed.toString() + ' when ' +
                             'have no record for it.\n');
         }


         //actually perform cleanup later so that don't instantly
         //re-use same id
         system.timeout(5,
                        std.core.bind(
                            function()
                            {
                                knownUsed[this.indexUsed] = null;
                                this.cb                   = null;
                                this.type                 = null;
                                this.message              = null;
                                this.additional           = null;                                
                            },this));
     };
     

     /**
      We want to recycle the divs,etc. associ
      */
     function addToKnownUsed(simpleInputObject)
     {
         var indexToUse = null;
         for (var s in knownUsed)
         {
             if (knownUsed[s] === null)
             {
                 indexToUse = s;
                 break;
             }
         }

         //could not find empty index.  Create new element
         if (indexToUse === null)
         {
             knownUsed.push(null);
             indexToUse = knownUsed.length - 1;
         }
         knownUsed[indexToUse] = simpleInputObject;
         return indexToUse;
     }


     function getSimpleInputGUIText()
     {
         var returner = "sirikata.ui('" + SIMPLE_INPUT_MOD_NAME + "',";
         returner += 'function(){ ';

         returner += @

         var newcsslink =
             $("<link />").attr({rel:'stylesheet', type:'text/css', href:'../scripting/prompt.css'});
	 $("head").append(newcsslink);

         //keeps track of all open windows requiring input events.
         var existingWindows = { };
                  

         function generateDivIDFromInputID(inputID)
         {
             return 'simpleInputID__' + inputID.toString();
         }

         function newWindow(htmlForWindow,simpleInputID)
         {
             var newWindowID = generateDivIDFromInputID(simpleInputID);
             $('<div>'   +
               htmlForWindow   +
               '</div>' //end div at top.
              ).attr({id:newWindowID,title:'simpleInput'}).appendTo('body');


             var inputWindow = new sirikata.ui.window(
                 '#' + newWindowID,
                 {
	             autoOpen: true,
	             height: 'auto',
	             width: 300,
                     height: 300,
                     position: 'right'
                 }
             );
             inputWindow.show();
             existingWindows[newWindowID] = [inputWindow,simpleInputID];
         }

         simpleInputSubmit = function(simpleInputID)
         {
             var returner = '';
             //first check to see if we got the value for a text element
             var textInputID = generateSimpleInputOptionID(simpleInputID);
             var textInput = $('#' + textInputID);
             if (textInput.length > 0)
             {
                 //means that we got a text input
                 returner =textInput.val();
             }
             else
             {
                 var radioInput = $('input:radio[name=' +
                                    textInputID +']:checked');
                 if (radioInput.length > 0)
                     returner = radioInput.val();
             }

             //automtaically handles case of noinput type of input by
             //passing undefined as responseReceived
             sirikata.event('responseReceived',simpleInputID,returner);                     

             

             //actually remove the window from view after its question has been answered.
             var inputID = generateDivIDFromInputID(simpleInputID);
             delete existingWindows[inputID];
             $('#'+inputID).remove();
         };


         clearSimpleInputWindow = function(simpleInputID)
         {
             var inputID = generateDivIDFromInputID(simpleInputID);
             if (inputID in existingWindows)
             {
                 delete existingWindows[inputID];
                 $('#' + inputID).remove();
             }
         };
         
         function generateSubmitButtonHtml(simpleInputID)
         {
             var htmlToDisplay = '';
             htmlToDisplay += '<button ' +
                 'onclick="simpleInputSubmit(' + simpleInputID.toString() +
                 ')">';
             
             htmlToDisplay += 'Submit';
             htmlToDisplay += '</button>';
             
             return htmlToDisplay;
         }
         
         simpleInputSelectList =
             function(message,simpleInputID,additional)
         {
             var htmlInsert = message + '<br/>';
             var radName = generateSimpleInputOptionID(simpleInputID);
             for (var s in additional)
             {
                 htmlInsert += '<input type=radio name="' +
                     radName + '" value="' + additional[s][1] +
                     '"></input>' + additional[s][0];
                 htmlInsert += '</br>';
             }
             htmlInsert += generateSubmitButtonHtml(simpleInputID);
             newWindow(htmlInsert,simpleInputID);
             sirikata.event('htmlSetupComplete',simpleInputID);
         };

         function generateSimpleInputOptionID(inputID)
         {
             return 'simpleInputIDOption__' + inputID.toString();
         }

         simpleInputNoInput = function (message,simpleInputID)
         {
             var htmlInsert = message + '<br/>';
             htmlInsert += generateSubmitButtonHtml(simpleInputID);
             newWindow(htmlInsert,simpleInputID);

             sirikata.event('htmlSetupComplete',simpleInputID);
         };

         
         simpleInputSelectText = function(message,simpleInputID)
         {
             var htmlInsert = message + '<br/>';
             htmlInsert += '<input id="'                      +
                 generateSimpleInputOptionID(simpleInputID)   +
                 '"></input><br/>';
             htmlInsert += generateSubmitButtonHtml(simpleInputID);
             newWindow(htmlInsert,simpleInputID);

             //handles enter submitting text
             var handleEnterHit = function(evt)
             {
                 //13 represents keycode for enter, submits whatever's in
                 //the text box if user hits enter.
                 if (evt.keyCode == 13)
                     simpleInputSubmit(simpleInputID);
             };

             //copied from chat.js
             var register_area = document.getElementById(
                 generateSimpleInputOptionID(simpleInputID));
             register_area.onkeyup = handleEnterHit;
             sirikata.event('htmlSetupComplete',simpleInputID);
         };
         @;


         returner += '});';         
         return returner;
     }
    
 }
)();

