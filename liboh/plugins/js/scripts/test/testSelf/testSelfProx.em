//If this works, then should see two separate sporefs out

system.print("\n\n");

function proxAddedCB()
{
    system.print('\nAdded to');
    system.print(system.self.toString());
    system.print('\n');
}

function proxRemovedCB()
{
    system.print('\nRemoved from');
    system.print(system.self.toString());
    system.print('\n');
}


var mMesh  = system.self.getMesh();
var mPos   = system.self.getPosition();
mPos = mPos + <2,0,0>;

presConnected();


function presConnected()
{
    system.self.setQueryAngle(3);
    system.self.onProxAdded(proxAddedCB);
    system.self.onProxRemoved(proxRemovedCB);
}

system.createPresence(mMesh,presConnected);
