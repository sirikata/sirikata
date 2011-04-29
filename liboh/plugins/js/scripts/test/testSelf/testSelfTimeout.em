//If this works, then should see two separate sporefs out

system.print("\n\n");

function timeoutCB()
{
    system.print("\n");
    system.print(system.self.toString());
    system.print("\n");
}


function presConnected()
{
    system.timeout(5,timeoutCB);
}

system.timeout(5,timeoutCB);

var mMesh  = system.self.getMesh();

system.createPresence(mMesh,presConnected);
