
//A bullet is associated with a duelist,
//who can fire the bullet and control the bullet's movement.  If the
//bullet is not fired, it moves with the duelist.  If the bullet is fired
//it moves in the direction indicated as an arg to the function shootFunction.
//If it "hits" an adversary (comes within BULLET_HIT_RADIUS) of the adversary
//while it is in flight, then it calls "kill" on that adversary.  If it does
//not hit anything, and is outside of MAX_BULLET_RADIUS of the duelist that
//fired it, it resets itself, moving back to its associated duelist.



system.import('std/core/bind.js');
system.import('std/movement/movement.em');


//how the bullet presence should look in the world.
var BULLET_MESH       = "meerkat:///danielrh/Insect-Bird.dae";
//when fired, bullet's translational speed
var BULLET_SPEED      =    20;
//when fired, how far the bullet can get away from
//the duelist before the bullet returns to the duelist
//and can be fired again.
var MAX_BULLET_RADIUS =   200;
//have no collision detection in the system now.  The hack around
//that is to say that when a bullet gets within BULLET_HIT_RADIUS
//of an adversary, that adversary is killed
var BULLET_HIT_RADIUS =    .1;



//Bullet constructor function. 
//duelistObj is of type Duelist
//startingPosition is of type Vec3, and indicates where in the world the bullet's
//presence should initially be connected to.
function Bullet(duelistObj,startingPosition)
{
    //Each bullet has a single pres in the world.  It uses this presence
    this.bulletPres    = system.create_presence(BULLET_MESH,startingPosition);

    //When this value is true, the bullet is in flight to its target.  Gets
    //reset to false when its traveled 200m away from duelist that fired it.
    this.isShot        = false;

    //duelist calls shootFunction to try to fire bullet.  If the bullet is
    //not already in flight (ie isShot == false), then, sends the bullet in
    //a direction specified as an arg to shootFunction.
    this.shootFunction = std.core.bind(bulletShootFunction,this);

    //the duelist associated with this bullet.  When the bullet is not
    //in flight (ie isShot == false), the bullet should follow its duelist
    //around the world.  (Think about it like a sherrif with a gun full of bullets.
    //When the sherrif hasn't fired, and he/she walks around the world, his/her bullets
    //move with him.
    //This "moving with" behavior is accomplished by the duelist's piping its movement
    //commands into the bulletMove function.
    this.mDuelist      = duelistObj;

    //interface to move the bullet's presence.  takes in the same msg object
    //that duelist uses to control it.  
    this.move          = std.core.bind(bulletMove,this);

    //called when bullet has travelled 200m from shooter, and not hit anything.
    //just returns the bullet back to the shooter and changes its state
    //back to not being shot (ie resets isShot to false).
    this.reset         = std.core.bind(bulletReset,this);

    //An array of all when statements associated with this bullet object
    //If a bullet hits something, or resets itself, clears all when statements.
    this.existingWhens = new Array();
    this.clearExistingWhens = std.core.bind(bulletClearExistingWhens,this);
    this.addToExistingWhens = std.core.bind(bulletAddToExistingWhens,this);


    //aliasing setPosition and setVelocity commands
    this.setPosition   = this.bulletPres.setPosition;
    this.setVelocity   = this.bulletPres.setVelocity;
    //aliasing isConnected.  Because booleans are vals
    //instead of objects in js, cannot just set to this.bulletPres.isConnected
    //as done in above two lines.
    this.isConnected   = false;
    
    when (bulletPres.isConnected)
    {
        this.isConnected = true;
    }
}


//Function to request the bullet to move.  Should be called by the duelist
//when he/she moves in the world so that bullet will follow his/her movements.
//Moves bullet if bullet is not currently in flight.
//bulletToMove is a Bullet object
//mvMsg is an object with the same form as movement messages that the duelist receives
//translationalSpeed is a number indicating how fast the bullet should move if mvMsg is
//a translation request.
//rotationalSpeed is a number indicating how fast the bullet should rotate if mvMsg is
//a rotation request.
function bulletMove(bulletToMove,mvMsg,translationalSpeed,rotationalSpeed)
{
    //do not move the bullet if it already is shot.
    if (bulletToMove.isShot)
        return;

    if (mvMsg.hasOwnProperty('forward'))
        std.movement.move(bulletToMove.bulletPres, new util.Vec3(1, 0, 0),translationalSpeed);
    else if (mvMsg.hasOwnProperty('backward'))
        std.movement.move(bulletToMove.bulletPres, new util.Vec3(-1, 0, 0),translationalSpeed);
    else if (mvMsg.hasOwnProperty('left'))
        std.movement.rotate(bulletToMove.bulletPres, new util.Vec3(0, 1, 0),rotationalSpeed);
    else if (mvMsg.hasOwnProperty('right'))
        std.movement.rotate(bulletToMove.bulletPres, new util.Vec3(0, -1, 0),rotationalSpeed);
    else if (mvMsg.hasOwnProperty('stop'))
    {
        std.movement.stopMove(bulletToMove.bulletPres);
        std.movement.stopRotate(bulletToMove.bulletPres);
    }
    else
        system.print("Error.  Cannot parse move message sent from controller in bullet.");

}



//manages existingWhens list of bulletObj by adding
//whenToAdd (of type when) to the end of the existingWhens list
function bulletAddToExistingWhens(bulletObj,whenToAdd)
{
    bulletObj.existingWhens.push(whenToAdd);
}

//manages existingWhens list of bulletObj by cancelling
//all whens and clearing the existingWhens array.
function bulletClearExistingWhens(bulletObj)
{
    for (var whenElement in bulletObj.existingWhens)
        whenElement.cancel();

    bulletObj.existingWhens.clear();
}

//Clears all whens, resets position of bullet to that of
//controlling duelist, and changes isShot flag to false
//to specify that the bullet is not in flight.
//should be called when a shot bullet has traveled 200m
//from its shooting duelist.
function bulletReset(bulletObj)
{
    bulletObj.clearExistingWhens();
    bulletObj.setPosition(bulletObj.mDuelist.masterPres.getPosition());
    bulletObj.setVelocity(bulletObj.mDuelsit.masterPres.getVelocity());
    bulletObj.isShot  = false;
}


//Called from duelist.  If the bullet is not already in flight (ie isShot == false),
//then, changes the velocity of the bullet so that it moves in targetDirection with
//speed BULLET_SPEED.
//Sets when statments so that if bulletObject hits adversary during that trip, it calls kill
//on that adversary.  Sets when statment so that if bullet travels 200m from duelist that fired
//it, the bullet is reset back to the duelist that fired it.
//bulletObject is of type Bullet
//targetDirection is a quaternion.  Bullet turns that direction, then goes straight.
//adversary is of type Duelist.
function bulletShootFunction(bulletObject,targetDirection,adversary)
{
    //cannot fire a bullet that's already in flight.
    if (bulletObject.isShot)
    {
        system.print("Cannot fire another bullet until already fired bullet is out of range.  Taking no action.");
        return;
    }

    //set the velocity of the bullet object to fire in targetDirection.
    std.movement.move(bulletObj,targetDirection,BULLET_SPEED);




    //when the bullet has traveled more than MAX_BULLET_RADIUS from where it was
    //fired, reset it.
    //Read as: get the bullet's position now (bulletObject.getPosition()).  When the bullet
    //goes from being within MAX_BULLET_RADIUS of that position to being outside of MAX_BULLET_RADIUS
    //of its initial position, execute the internal code block.
    var whenOutOfBounds = when(util.create_distGT(bulletObj.bulletPres, `bulletObject.getPosition()`, MAX_BULLET_RADIUS))
    {
        bulletObject.reset();
    };

    //note: do not have a way for canceling a when from within itself or for canceling stale
    //whens.  So pushing when statement onto an array of existingWhens. after when statement
    //if it has not been executed, then push it onto existingWhens.  Otherwise, cancel it.
    if (! whenOutOfBounds.getPredState())
        bulletObject.existingWhens.push(whenOutOfBounds);            
    else
        whenOutOfBounds.cancel();


    
    //if bullet does not yet have an adversary, then don't have to
    //worry about case of what to do when hits adversary.
    if (adversary == null)
        return;


    //handles case of bullet hits adversary.
    var whenHits = when (util.create_distLT(bulletObj.bulletPres,adversary.masterPres,BULLET_HIT_RADIUS))
    {
        adversary.kill();
        bulletObject.reset();
    };

    if (! whenHits.getPredState())
        bulletObject.existingWhens.push(whenHits);
    else
        whenHits.cancel();
}


