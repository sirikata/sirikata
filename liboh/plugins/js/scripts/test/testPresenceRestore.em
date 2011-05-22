//This script creates a new presence, and then sets the velocity of
//that presence so that it drifts away.


var presData = null;

//newPres contains the new presence that was connected
function newPresCallback(newPres)
{
    system.print("\n\nNew presence is connected!\n\n");

    //tell the new presence to drift away with velocity 1 in the
    //x direction.
    var disconFunc = function()
    {
        system.print('\n\nDisconnecting\n\n');
        presData = system.self.getAllData();
        system.prettyprint(presData);
        system.self.disconnect();
        system.timeout(5,restoreFunc);
    };

    system.timeout(5,disconFunc);
}

//new presences require a mesh.  Using mesh associated
//with already-existing first presence
var myMesh = system.self.getMesh();


//Two arguments:
//   first argument: mesh that the new presence should have
//   second arg:     a function specifying what the presence
//                   should do once it's connected to the space
system.createPresence(myMesh,newPresCallback);



function restoreFunc()
{
     /**
      @param {string} sporef,
      @param {vec3} pos,
      @param {vec3} vel,
      @param {string} posTime,
      @param {quaternion} orient,
      @param {quaternion} orientVel,
      @param {string} orientTime,
      @param {string} mesh,
      @param {number} scale,
      @param {boolean} isCleared ,
      @param {uint32} contextId,
      @param {boolean} isConnected,
      @param {function,null(if hasCC is false)} connectedCallback,
      @param {boolean} isSuspended,
      @param {vec3} suspendedVelocity,
      @param {quaternion} suspendedOrientationVelocity,
      */
    system.restorePresence(presData.sporef,
                           presData.pos,
                           presData.vel,
                           presData.posTime,
                           presData.orient,
                           presData.orientVel,
                           presData.orientTime,
                           presData.mesh,
                           presData.scale,
                           presData.isCleared ,
                           presData.contextId,
                           presData.isConnected,
                           function()
                           {
                               system.presences[0].setPosition(system.presences[0].getPosition() + <2,0,0>);
                           },
                           presData.isSuspended,
                           presData.suspendedVelocity,
                           presData.suspendedOrientationVelocity
                          );
    
}

