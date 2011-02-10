

var myMesh = system.presences[0].getMesh();

function newPresCallback()
{
    system.print("\n\nGot into newPresCallback\n\n");
    var rootPos = system.presences[0].getPosition();
    rootPos.x = rootPos.x + 2;
    system.presences[1].setPosition(rootPos);

    var newPos = system.presences[1].getPosition();
    var newPosString = newPos.toString();
    system.print("\n\nThis is the position of the new presence: " + newPosString + "\n\n");
    
}

system.create_presence(myMesh,newPresCallback);
