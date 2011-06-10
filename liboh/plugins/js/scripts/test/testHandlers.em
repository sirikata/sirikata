//Should be run on two entities to test handler and pattern matching.

//Whenever you receive a message from another presence, you set up a
//timeout so that you send them keep-alive messages every 10 seconds.  Forever.
//Also sets up a particular handler for the presence that it receives the
//message from that specifically prints out the message it receives.

//To bootstrap, when initially run, sets query angle to be tiny.  Then on
//each proxAdded, sends a message to an object.

//will eventually be put into a nice library.

function callbackHelper()
{
    system.print("\ngot into callback func\n");
    this.userCallback();
    this.timer = system.timeout(this.period,this.callback);
    system.print('\n\n');
}


function RepeatingTimer(period, callback)
{
    this.period       = period;
    this.userCallback = callback;
    this.callback     = std.core.bind(callbackHelper,this);
    this.timer        = system.timeout(this.period,this.callback);
    this.suspend   = function ()
    {
        this.timer.suspend();
    };
    this.resume    = function ()
    {
         this.timer.resume();  
    };
}

//



var knownArray = new Array();
var allSeen    = new Array();
//constructs a dummy message and sends it to toWhom
function sendMessage(toWhom)
{
    var msgToSend = new Object();
    msgToSend.msg = "Blah message";

    toWhom.sendMessage(msgToSend);
}

//when see a new presence, instantly send it a message.
//and add it to a list of objects that I've seen
function onProxAddedCallback(newProx)
{
    sendMessage(newProx);
    allSeen.push(newProx);
}

//what to do with msgs that we receive from presences that
//are in the knownArray
function knownMsgHandler(msg,msgFrom)
{
    system.print("\nReceived a message from a known handler.\n");;
}

//if do not already have this presence in the knownArray, add it.
function addKnownArray(msgFrom)
{
    for (var s in knownArray)
    {
        if (msgFrom.checkEqual(knownArray[s]))
        {
            return false;                
        }
    }

    knownArray.push(msgFrom);
    return true;
}

//what to do when receive any message.
function msgHandler(msg,msgFrom)
{
    if (addKnownArray(msgFrom))
    {
        knownMsgHandler << new util.Pattern() <- msgFrom;
        system.print("\nadding a new communicator\n");
        return;
    }
}

msgHandler << new util.Pattern();


function sendToAllSeen()
{
    system.print("\nin send to all seen\n");
    for (var s in allSeen)
    {
        sendMessage(allSeen[s]);
    }
}


system.presences[0].onProxAdded(onProxAddedCallback);
system.presences[0].setQueryAngle(.00001);

var mTimer =  RepeatingTimer(10,sendToAllSeen);