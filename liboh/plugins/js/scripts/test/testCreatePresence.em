

var myMesh = system.presences[0].getMesh();

function newPresCallback()
{
    system.print("\nGot into newPresCallback\n");
    var rootPos = system.presences[0].getPosition();
    rootPos.x = rootPos.x + 2;
    system.presences[1].setPosition(rootPos);
}

system.create_presence(myMesh,newPresCallback);
