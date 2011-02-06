


function proxAddedCallback(visObj)
{
    var myPos = system.presences[0].getPosition();

    var distToObj = visObj.dist(myPos);
    var distToObjString = distToObj.toString();
    system.print("\n\nThis is the distance to that object from me: " + distToObjString + "\n\n");
    
}



system.presences[0].onProxAdded(proxAddedCallback);
system.presences[0].setQueryAngle(.01);