//If this works, then should see two separate sporefs out

system.print("\n\n");
system.print(system.self.toString());

function presConnected()
{
    system.print('\n\n');
    system.print(system.self.toString());
    system.print('\n\n');
}


var mMesh  = system.self.getMesh();
var mPos   = system.self.getPosition();
mPos = mPos + <2,0,0>;


system.createPresence(mMesh,presConnected);
