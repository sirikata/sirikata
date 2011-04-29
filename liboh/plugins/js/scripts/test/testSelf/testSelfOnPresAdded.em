//this tests whether self is being mapped correctly for onPresenceConnected callbacks
//If everything works correctly, you should see printed in the terminal two different space and refs.


system.print("\n\n");
system.print(system.self.toString());

function presConnected()
{
    system.print('\n\n');
    system.print(system.self.toString());
    system.print('\n\n');
}

system.onPresenceConnected(presConnected);


var mMesh  = system.self.getMesh();
var mPos   = system.self.getPosition();
mPos = mPos + <2,0,0>;


system.createPresence(mMesh,function(){});





