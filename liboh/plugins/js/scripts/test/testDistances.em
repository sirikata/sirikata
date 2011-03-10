
function proxCallback(newVisObj)
{
    var dist = distanceFromMeToIt(newVisObj);

    system.print("\n\nThis is the distance\n");
    system.print(dist);
    system.print("\t");
    system.print(system.presences[0].getPosition());
    system.print("\t");
    system.print(newVisObj.getPosition());
    system.print("\n\n");
    
}


function distanceFromMeToIt(it)
{
    var myPosition  = system.presences[0].getPosition();
    var itsPosition = it.getPosition();


    var diffX =     (myPosition.x - itsPosition.x);
    var diffY =     (myPosition.y - itsPosition.y);
    var diffZ =     (myPosition.z - itsPosition.z);
    
    var diffXSquared = diffX * diffX;
    var diffYSquared = diffY * diffY;
    var diffZSquared = diffZ * diffZ;
    var sumOfSquares = diffXSquared + diffYSquared + diffZSquared;

    var distance = util.sqrt(sumOfSquares);
    
    return distance;
}

system.presences[0].onProxAdded(proxCallback);
//system.presences[0].setQueryAngle(.001);
system.presences[0].setQueryAngle(4);
