

var myMesh = system.presences[0].getMesh();

function newPresMove(presToMove)
{
    presToMove.setVelocity(new util.Vec3(-.1,0,0));
}

function newPresCallback()
{
    system.print("\n\nGot into newPresCallback\n\n");
    var rootPos = system.presences[0].getPosition();
    rootPos.x = rootPos.x + 2;
    system.presences[1].setPosition(rootPos);

    var newPos = system.presences[1].getPosition();
    var newPosString = newPos.toString();
    system.print("\n\nThis is the position of the new presence: " + newPosString + "\n\n");


    system.timeout(5,function(){
                       newPresMove(system.presences[1]);
                                  });


    var lenPresArray = system.presences.length;
    var lenPresArrayString = lenPresArray.toString();
    system.print("\n\nPrinting length of array string: " + lenPresArrayString + "\n\n");
    
}

system.create_presence(myMesh,newPresCallback);
