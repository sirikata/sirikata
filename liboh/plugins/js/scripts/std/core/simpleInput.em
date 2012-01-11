
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
         knownUsed = null;
         inputter.cb(response);
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
      */
     std.core.SimpleInput = function(type, message, cb, additional)
     {
         if (!haveInited)
             throw new Error('Error.  GUI still initializing.  Please wait.');

         if (type == std.core.SimpleInput.SELECT_LIST)
         {
             throw new Error('Error.  Have not finished setting html ' +
                             'in simpleInputSelectList function.  '    +
                             'Must complete.');
         }
         
         this.type       = type;
         this.message    = message;
         this.cb         = cb;
         this.additional = additional;

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

     std.core.SimpleInput.ready = function()
     {
         return haveInited;
     };

     
     /**
      We want to recycle the divs,etc. associ
      */
     function addToKnownUsed(simpleInputObject)
     {
         var indexToUse = null;
         for (var s in knownUsed)
         {
             if (knownUsed[s] == null)
             {
                 indexToUse = s;
                 break;
             }
         }

         //could not find empty index.  Create new element
         if (indexToUse == null)
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
             var returner = null;
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

             
             if (returner === null)
             {
                 sirikata.log('error','In simpleInput, selecting ' +
                              'an input that does not exist.');
             }
             else
             {
                 sirikata.event('responseReceived',simpleInputID,returner);                     
             }

             
             delete existingWindows[simpleInputID];
             //actually remove the window from view after its question has been answered.
             var inputID = generateDivIDFromInputID(simpleInputID);
             $('#'+inputID).remove();
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
             newWindow(htmlInsert,simpleInputID);
         };

         function generateSimpleInputOptionID(inputID)
         {
             return 'simpleInputIDOption__' + inputID.toString();
         }
         
         simpleInputSelectText = function(message,simpleInputID)
         {
             var htmlInsert = message + '<br/>';
             htmlInsert += '<input id="'                      +
                 generateSimpleInputOptionID(simpleInputID)   +
                 '"></input><br/>';
             htmlInsert += generateSubmitButtonHtml(simpleInputID);
             newWindow(htmlInsert,simpleInputID);
         };
         
         @;


         returner += '});';         
         return returner;
     }
    
 }
)();

