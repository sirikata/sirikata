
//DuelingGame creates a presence in the virtual world.  Subscribers
//(likely avatars in the virtual world) can send messages to this presence
//to register for the game.  DuelingGame only allows two subscribers to
//play the game, and creates a Duelist object for each.  Each subscriber
//can control his/her duelist (request it to move and fire) by sending
//additional messages either to it or to the DuelingGame itself.


system.import('std/core/bind.js');
system.import('test/duel/duelist.em');

//default mesh to load for dueling game
var DUELING_GAME_MESH = "meerkat:///danielrh/Poison-Bird.dae";



//Constructor for DuelingGame.
//takes in a single parameter: duelingGameStartPos, which
//is a Vec3 indicating where the presence associated with the
//dueling game should appear in the world.
function DuelingGame(duelingGameStartPos)
{
    //physical manifestation of the game in the virtual world.  In an actual game,
    //this may be something like a sign up booth, a kiosk, etc. 
    this.mPres = system.create_presence(DUELING_GAME_MESH, duelingGameStartPos);

    //initialization function is executed once when mPres initially connects
    //sets up handlers for subscription messages, described below.
    this.initialize = std.core.bind(duelingGameInitialize,this);

    //aliases the getPosition function of the presence associated with the dueling game
    this.getPosition = this.mPres.getPosition();

    //dueling game has two duelists, A and B.  Each starts as null.
    //Upon receiving a subscription message, the dueling game populates
    //first duelerA and then duelerB
    this.duelerA  = null;
    this.duelerB  = null;

    //callback function to be dispatched when receive a subscription request
    //meessage.  The subscriptionCallback is initially bound in the initialization
    //function of the dueling game object.
    this.subscriptionCallback = std.core.bind(duelingGameSubscriptionCallback,this);

    //upon receiving a subscription message, call subJoinResponse if have room for
    //subscriber (ie either duelerA or duelerB is null).  subJoinResponse sends
    //a message back to the subscriber indicating success.  If do not have room
    //for subscriber (ie bother duelerAn and duelerB are populated), call subNoJoinResponse
    //which tells the potential subscriber that there is no room for him/her
    this.subJoinResponse      = std.core.bind(duelingGameSubJoinResponse,this);
    this.subNoJoinResponse    = std.core.bind(duelingGameSubNoJoinResponse,this);

    //simple utility for sending a message object from this.mPres to a specified
    //receiver.
    this.sendMessage          = std.core.bind(duelingGameSendMessage, this);
    

    //when mPres is first connected, execute initialization code
    when (this.mPres.isConnected)
    {
        this.initalize();
    }
}


//duelingGameObj is of type DuelingGame
//Calling initialize registers the duelingGameObj's subscriptionCallback
//to fire when we receive any message that has a field "subscribe" in it.
function duelingGameIntialize(duelingGameObj)
{
    duelingGameObj.subscriptionCallback  <- new util.Pattern("subscribe");
}


//This callback is dispatched when receive a subscription request
//message from msgSender.  If do not already have two duelists,
//populate a duelist, and associate it with msgSender.  Further,
//notify msgSender that his/her subscription was successful.
//If already have two duelists, notify msgSender that subscription
//was unsuccesful.
//duelingGameObj is of type DuelingGame
//subscriptionMsg is an arbitrary object representing a message that msgSender
//sent to duelingGameObj.mPres.
//msgSender is of type VisibleObject
function duelingGameSubscriptionCallback(duelingGameObj, subscriptionMsg, msgSender)
{

    //always populates duelerA before duelerB.
    if (duelingGameObj.duelerA == null)
    {
        //populate duelerA
        var newDuelerName = 'A';
        duelingGameObj.duelerA = new Duelist (msgSender, newDuelerName, duelingGameObj.getPosition());

        //send response to subscriber that subscription was successful.
        duelingGameObj.subJoinResponse(msgSender,newDuelerName);
    }
    else if (duleingGameObj.duelerB == null)
    {
        //populate duelerB
        var newDuelerName = 'B';
        duelingGameObj.duelerB = new Duelist (msgSender, newDuelerName, duelingGameObj.getPosition());

        //send response to subscriber that subscription was successful.
        duelingGameObj.subJoinResponse(msgSender,newDuelerName);

        //at this point have all duelists.  Register each duelist as an
        //adversary of the other and proceed.
        duelingGameObj.duelerA.registerAdversary(duelingGameObj.duelerB);
        duelingGameObj.duelerB.registerAdversary(duelingGameObj.duelerA);
    }
    else
    {
        //all duelists were full.  Message potential subscriber that
        //subscription failed
        duelingGameObj.subNoJoinResponse(msgSender);
    }


}


//Creates a message object that subscription for dueler with name duelerName
//was successful.  Then sends that messageObject to toSendTo through the presence
//associated with duelingGameObj
//duelingGameObj is of type DuelingGame
//toSendTo is of type Visible
//duelername is assumed to be of type String.
function duelingGameSubJoinResponse(duelingGameObj,toSendTo, duelerName)
{
    //creates subscription success message
    var msgObj          =   new Object();
    msgObj.subscription =   "registered";
    msgObj.duelerName   =     duelerName;

    //duelingGameObj sends the message msgObj to toSendTo
    duelingGameObj.sendMessage(toSendTo,msgObj);
}


//Creates a message object that subscription for dueler with name duelerName
//was not successful.  Then sends that messageObject to toSendTo through the presence
//associated with duelingGameObj
//duelingGameObj is of type DuelingGame
//toSendTo is of type Visible
function duelingGameSubNoJoinResponse(duelingGameObj,toSendTo)
{
    //creates subscription failure message
    var msgObj = new Object();
    msgObj.subscription = "failed";

    //duelingGameObj sends the message msgObj to toSendTo
    duelingGameObj.sendMessage(toSendTo,msgObj);
}


//sends a message, msgToSend, from the presence associated with duelingGameObj
//to toSendTo
//duelingGameObj is of type DuelingGame
//toSendTo is of type Visible
//msgToSend is an object
function duelingGameSendMessage(duelingGameObj,toSendTo, msgToSend)
{
    toSendTo.sendMessage(duelingGameObj.mPres,msgToSend);
}
