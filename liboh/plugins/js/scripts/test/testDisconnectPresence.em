//This script creates a new presence, and then sets the velocity of
//that presence so that it drifts away.


//newPres contains the new presence that was connected
function newPresCallback(newPres)
{
    system.print("\n\nNew presence is connected!\n\n");

    //tell the new presence to drift away with velocity 1 in the
    //x direction.
    var disconFunc = function()
    {
        system.print('\n\nDisconnecting\n\n');
        system.self.disconnect();        
    };

    system.timeout(10,disconFunc);
}

//new presences require a mesh.  Using mesh associated
//with already-existing first presence
var myMesh = system.self.getMesh();


//Two arguments:
//   first argument: mesh that the new presence should have
//   second arg:     a function specifying what the presence
//                   should do once it's connected to the space
system.createPresence(myMesh,newPresCallback);
