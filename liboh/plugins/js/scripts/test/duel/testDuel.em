//when the
system.import('test/duel/bullet.em');
system.import('test/duel/duelist.em');


var DUELING_START_POS = new util.Vec3(0,0,0);


var duelingGamePres = system.create_presence(DUELING_GAME_MESH, DUELING_START_POS);

when (duelingGamePres.isConnected)
{
    
    
}


system.onPresenceConnected(setupDueling);

function setupDueling()


system.import('std/core/bind.js');
system.import('std/movement/movement.em');

var BULLET_SPEED      = 20;
var MAX_BULLET_RADIUS = 200;
var BULLET_HIT_RADIUS =  .1;
var numDuelists   = 0;
var arrayDuelists = new Array();


//this function sets up handlers for messages that control
//the duelist's movements and whether to fire.
function duelistInitializationFunc(duelist, controller)
{
    bindShoot(duelist, controller);
    bindMoveMsgs(duelist,controller);
}


//targetDirection is a quaternion relative to the current direction
//duelist is facing.
function bulletShootFunction(bulletObject,targetDirection,adversary)
{
    if (bulletObject.isShot)
    {
        system.print("Cannot fire another bullet until already fired bullet is out of range.  Returning.");
        return;
    }


    ns.move(bulletObj,targetDirection,BULLET_SPEED);


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


function Bullet(duelistObj)
{
    this.bulletPres    = system.create_presence();
    this.setPosition   = bulletPres.setPosition;
    this.setVelocity   = bulletPres.setVelocity;
    this.isConnected   = false;
    this.isShot        = false;
    this.shootFunction = std.core.bind(bulletShootFunction,this);
    this.mDuelist      = duelistObj;
    this.existingWhens = new Array();
    this.clearExistingWhens = lkjs;
    
    when (bulletPres.isConnected)
    {
        this.isConnected = true;
    }
}

//an individual duelist has a single presence.
function Duelist(controller,duelistIdentifier)
{
    this.masterPres        = system.create_presence(); lkjs;
    this.bullet            =             new Bullet(this);
    this.stillAlive        =                     true;
    this.duelistIdentifier =        duelistIdentifier;
    this.initialize        = std.core.bind(duelistInitializationFunc,this,controller); lkjs;
    this.kill              =        duelistKill; lkjs;
    
    when (masterPres.isConnected && bullet.isConnected)
    {
        this.initialize();
    }
}


function createDuelist(duellistController)
{
    var newDuelist = new Duelist(duellistController,numDuelists++);
    arrayDuelists.push(newDuelist);

}



