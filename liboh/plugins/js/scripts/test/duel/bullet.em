system.import('std/core/bind.js');
system.import('std/movement/movement.em');


var BULLET_SPEED      =    20;
var MAX_BULLET_RADIUS =   200;
var BULLET_HIT_RADIUS =    .1;
var BULLET_MESH       = "meerkat:///danielrh/Insect-Bird.dae";


function Bullet(duelistObj,startingPosition)
{
    this.bulletPres    = system.create_presence(BULLET_MESH,startingPosition);
    this.setPosition   = this.bulletPres.setPosition;
    this.setVelocity   = this.bulletPres.setVelocity;
    this.isConnected   = false;
    this.isShot        = false;
    this.shootFunction = std.core.bind(bulletShootFunction,this);
    this.mDuelist      = duelistObj;

    this.move          = std.core.bind(bulletMove,this);
    this.reset         = std.core.bind(bulletReset,this);
    
    this.existingWhens = new Array();
    this.clearExistingWhens = std.core.bind(bulletClearExistingWhens,this);
    this.addToExistingWhens = std.core.bind(bulletAddToExistingWhens,this);
    
    when (bulletPres.isConnected)
    {
        this.isConnected = true;
    }
}

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


function bulletAddToExistingWhens(bulletObj,whenToAdd)
{
    bulletObj.existingWhens.push(whenToAdd);
}

function bulletClearExistingWhens(bulletObj,whenToRemove)
{
    for (var whenElement in bulletObj.existingWhens)
        whenElement.cancel();

    bulletObj.existingWhens.clear();
}

function bulletReset(bulletObj)
{
    bulletObj.clearExistingWhens();
    bulletObj.setPosition(bulletObj.mDuelist.masterPres.getPosition());
    bulletObj.setVelocity(bulletObj.mDuelsit.masterPres.getVelocity());
    bulletObj.isShot  = false;
}


//targetDirection is a quaternion relative to the current direction
//duelist is facing.
function bulletShootFunction(bulletObject,targetDirection,adversary)
{
    if (bulletObject.isShot)
    {
        system.print("Cannot fire another bullet until already fired bullet is out of range.  Taking no action.");
        return;
    }


    std.movement.move(bulletObj,targetDirection,BULLET_SPEED);


    var whenCondMet = false;
    //note: do not have strong semantics for cancelling whens.
    var whenOutOfBounds = when(util.create_distGT(bulletObj.bulletPres, bulletObject.getPosition(), MAX_BULLET_RADIUS))
    {
        bulletObject.reset();
        whenCondMet = true;
    };

    if (whenCondMet)
        return;        

    bulletObject.existingWhens.push(whenOutOfBounds);            
    

    if (adversary == null)
        return;
    
    //handles case of bullet hits adversary.
    var whenHits = when (util.create_distLT(bulletObj.bulletPres,adversary.masterPres,BULLET_HIT_RADIUS))
    {
        adversary.kill();
        bulletObject.reset();
        whenCondMet = true;
    };

    if (whenCondMet)
        return;

    bulletObject.existingWhens.push(whenHits);
}


