
var turnLeft = true;
var amountToMoveLeft  = -5;
var amountToMoveRight = 5;

function leftCallback()
{
    system.print("\n\nBegin move left\n\n");
    system.presences[0].setVelocity(new system.Vec3(amountToMoveLeft,0,0));
    system.timeout(2,null,stopCallback);
    turnLeft = false;
}

function rightCallback()
{
    system.print("\n\nBegin move right \n\n");
    system.presences[0].setVelocity(new system.Vec3(amountToMoveRight,0,0));

    system.timeout(2,null,stopCallback);
    turnLeft = true;
}


function stopCallback()
{
    system.print("\n\nBegin move stop\n\n");
    system.presences[0].setVelocity(new system.Vec3(0,0,0));

    if (turnLeft)
    {
        system.timeout(1,null,leftCallback);
    }
    else
    {
        system.timeout(1,null,rightCallback);                        
    }
}


system.timeout(2,null,leftCallback);



