//assume have already gotten logged into space

//HIGH-LEVEL explanation:
//This file is to test the getPosition and setPosition functions fromx presences.
//Whenever someone sends the scripted object a message with an object that has a field
//called "tremble", we call the tremble function.

//clear whatever else may have been here.

//system.reboot();


system.print("\n\n\n\nIMPORTING TEST SET POSITION FILE\n\n");

centerPosition      =      system.presences[0].getPosition();
awayFromCenterBool  =                                  false;

system.print("\n\nThis is the centerPosition.x: \n");
system.print(centerPosition.x);
system.print("\n\n");

displacementVector =            new system.Vec3(1.0,1.0,1.0);


function trembleCallbackFunction()
{
    var currentPosition = system.presences[0].getPosition();
    var newPos;
    
    if (awayFromCenterBool)
    {
        newPos = addVecs(currentPosition , displacementVector);
        awayFromCenterBool = false;
    }
    else
    {
        newPos = subtractVecs(currentPosition , displacementVector);
        awayFromCenterBool = true;
    }

    system.print("\n\n\n\nCALLBACK CALLED: here are new x,y,z positions:\n\n");    
    system.print("\n");
    system.print(newPos.x);
    system.print("\n");
    system.print(newPos.y);
    system.print("\n");
    system.print(newPos.z);
    
    system.presences[0].setPosition(newPos);

}

function addVecs(vecA,vecB)
{
    var returner = system.Vec3(vecA.x+vecB.x, vecA.y+vecB.y, vecA.z+vecB.z);
    return returner;
}

function subtractVecs(vecA,vecB)
{
    var returner = system.Vec3(vecA.x-vecB.x, vecA.y-vecB.y, vecA.z-vecB.z);
    return returner;
}


var matchTremblePattern = new system.Pattern("tremble");

var trembleHandler      = system.registerHandler(matchTremblePattern,null,trembleCallbackFunction,null);

var objer = new Object();
objer.tremble = true;

function callTrembler()
{
    system.__broadcast(objer);
}



