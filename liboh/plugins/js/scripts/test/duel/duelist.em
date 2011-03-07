system.import('std/core/bind.js');
system.import('std/movement/movement.em');
system.import('std/test/duel/bullet.em');

//what default mesh the presence associated with a duelist should have.
var DUELIST_MESH                = "meerkat:///danielrh/Poison-Bird.dae";
//scaling factor indicating how fast a duelist should move when
//it receives a message to move translationally (ie, non-rotation).
//higher is faster.  
var DUELIST_TRANSLATIONAL_SPEED =                                     5;
//scaling factor factor indicating how fast a duelist should rotate
//when receivs a message to rotate.
//higer is faster
var DUELIST_ROTATIONAL_SPEED    =                                     1;


//Each duelist is a presence in the world that is controlled by an external
//controller.  The external controller can move the duelist and request the
//duelist to "shoot" by sending messages to the duelist.
//Each duelist has a single bullet.  See description of bullet in bullet.em.
//A duelist can be killed when a bullet hits him/her.



//Constructor for duelist objects.
//controller is of type Visible.  Duelist will only respond to messages (movement
//or requests to fire) if sent from controller.
//duelistIdentifier is of type string.  Used to uniquely specify duelist.
//startingPosition is of type Vec3.  Specifies where to initially instantiate
//duelist in the world.
function Duelist(controller,duelistIdentifier,startingPosition)
{
    //each duelist has a presence in the world.  This presence can
    //be moved by controller.
    this.masterPres        = system.create_presence(DUELIST_MESH,startingPosition);

    //each duelist has a single bullet.  If one duelist's bullet hits
    //another duelist, the bullet calls the "kill" function on the
    //hit duelist.
    this.bullet            =  new Bullet(this);

    //if a duelist has not been killed (hit by another duelist's bullet) he/she
    //is stillAlive.  When stillAlive is false, the duelist no longer responds
    //to any commands from its controller.
    this.stillAlive        =  true;

    //a unique identifier for the duelist to distinguish messages to the entity
    this.duelistIdentifier =  duelistIdentifier;

    //called when presence associated with this duelist is connected as well as
    //the duelist's bullet is connected
    this.initialize        =   std.core.bind(duelistInitializationFunc,this,controller);

    //when another duelist's bullet hits this duelist, the other duelist's bullet
    //calls the kill function on this duelist
    this.kill              = std.core.bind(duelistKill,this);

    //callback that the system dispatches if the controller sends
    //a message to this entity asking this duelist to shoot
    this.shootCallback     = std.core.bind(duelistShootCallback,this);

    //callback that the system dispatches if the controller sends
    //a message to this entity asking this duelist to move
    this.moveCallback      = std.core.bind(duelistMoveCallback,this);

    //Eventaully another duelist to try to kill.  
    this.adversary         = null;

    //Populates adversary field with another duelist to try to kill
    this.registerAdversary = std.core.bind(duelistRegisterAdversary,this);


    //when both the duelist and his/her bullet objects have been connected
    //to the world, issue the initialize command.
    when (masterPres.isConnected && bullet.isConnected)
    {
        this.initialize();
    }
}



//Sets up handlers for messages that control
//the duelist's movements and whether to fire.
//duelist is of type Duelist
//controller is of type Visible.
function duelistInitializationFunc(duelist, controller)
{
    //register shoot handlers
    //ie fire duelist.shootCallback if this entity receives a message object
    //from controller that has a field "duelist" that is equal to the duelist's
    //identifier *and* has a field "msgType" that is equal to the value "shoot".
    duelist.shootCallback
       <- [new util.Pattern("duelist",duelist.duelistIdentifier), new util.Pattern("msgType","shoot")]
       <- controller;
    

    //register move handlers
    duelist.moveCallback
       <- [new util.Pattern("duelist",duelist.duelistIdentifier), new util.Pattern("msgType","move")]
       <- controller;

}


//This callback fires when duelistObj's controller sends a request
//for the duelist to shoot.  All it does is request the duelist's
//bullet to shoot if the duelist is still alive.  
//duelistObj is of type Duelist
//shootMsg is an object that corresponds to the message that msgSender
//sent to this entity requesting duelistObj to shoot.
//msgSender is of type Visible (and should correspond to the controller
//of duelistObj).
function duelistShootCallback(duelistObj, shootMsg, msgSender)
{
    //a dead duelist cannot shoot.
    if (!duelistObj.stillAlive)
        return;

    
    //requests the duelist's bullet to shoot itself in the direction
    //of the duelist's current orientation.
    var currentOrientation = duelistObj.masterPres.getOrientation();
    bullet.shootFunction(currentOrientation,duelistObj.adversary);
}

//Fires when duelistObj's controller sends a request for the duelist
//to move.  Note in addition to moving the duelist him/herself, this 
//function also requests the bullet to move with the duelist.  (If the
//bullet is being fired, the bullet ignores this request.)

//duelistObj is of type Duelist
//mvMsg is an object that corresponds to the message that msgSender
//sent to this entity requesting duelistObj to move.
//msgSender is of type Visible (and should correspond to the controller
//of duelistObj.
function duelistMoveCallback(duelistObj,mvMsg,msgSender)
{
    //a dead duelist cannot move
    if (!duelistObj.stillAlive)
        return;

    //dispatches based on what type of movement is requested.
    //could have also specified each of these as additional fields
    //for message patterns to match, but thought manual dispatch
    //would be easier.
    if (mvMsg.hasOwnProperty('forward'))
        std.movement.move(duelistObj.masterPres, new util.Vec3(1, 0, 0),DUELIST_TRANSLATIONAL_SPEED);
    else if (mvMsg.hasOwnProperty('backward'))
        std.movement.move(duelistObj.masterPres, new util.Vec3(-1, 0, 0),DUELIST_TRANSLATIONAL_SPEED);
    else if (mvMsg.hasOwnProperty('left'))
        std.movement.rotate(duelistObj.masterPres, new util.Vec3(0, 1, 0),DUELIST_ROTATIONAL_SPEED);
    else if (mvMsg.hasOwnProperty('right'))
        std.movement.rotate(duelistObj.masterPres, new util.Vec3(0, -1, 0),DUELIST_ROTATIONAL_SPEED);
    else if (mvMsg.hasOwnProperty('stop'))
    {
        std.movement.stopMove(duelistObj.masterPres);
        std.movement.stopRotate(duelistObj.masterPres);
    }
    else
        system.print("Error.  Cannot parse move message sent from controller in duelist.");

    //request the bullet object to move with the duelist.  The basic idea is that a duelist's
    //bullets are on his/her person at all times (unless being fired).  If the bullet is
    //being fired, it ignores this movement request.
    duelistObj.bullet.move(mvMsg,DUELIST_TRANSLATIONAL_SPEED,DUELIST_ROTATIONAL_SPEED);
}


//populates the adversary field of duelistToRegWith with
//adversary.
//duelistToRegWith and adversary are both of type Duelist.
function duelistRegisterAdversary(duelistToRegWith, adversary)
{
    duelistToRegWith.adversary = adversary;
}

//When a duelist's adversary's bullet hits it, that bullet
//calls kill on the hit duelist.  The kill function changes the
//aliveness of the hit duelist and stops him/her from moving.
//duelistToKill is a Duelist object associated with the duelist that
//was hit by an adversary's bullet.
function duelistKill(duelistToKill)
{
    system.print("\nI've been killed!\n");
    duelistToKill.stillAlive = false;

    //stop the duelist from moving.
    std.movement.stopMove(duelistToKill.masterPres);
    std.movement.stopRotate(duelistToKill.masterPres);
}


