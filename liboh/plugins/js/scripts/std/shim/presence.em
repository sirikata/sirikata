/*  Sirikata
 *  presence.em
 *
 *  Copyright (c) 2011, Bhupesh Chandra
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Sirikata nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


/* Add functions to the prototype for presence */

system.shim = new Object();
system.__presence_constructor__.prototype.runSimulation =
function(name)
{
  system.print("\n\nIn side the runSimulation\n\n");
  var pres = this;
  if(!system.shim.graphics)
  {
      system.print("\n\n Got the new graphics object\n\n");
      system.shim.graphics = new Object();
  }
  if(!system.shim.graphics[pres])
  {
    system.shim.graphics[pres] = new Object();
    system.print("\n\nBefore _runSimulation\n\n");
    system.shim.graphics[pres][name] = pres._runSimulation(name);
  }

  var return_val = system.shim.graphics[pres][name];

	return return_val;
};

system.__presence_constructor__.prototype.getSimulation =
function(name)
{

  if(!system.shim.graphics || !system.shim.graphics[this]) return undefined;
  return system.shim.graphics[this][name];
};

system.__presence_constructor__.prototype.onProxAdded =
function (funcToCallback)
{
    system.__sys_onProxAdded(this,funcToCallback);
};

system.__presence_constructor__.prototype.onProxRemoved =
function (funcToCallback)
{
    system.__sys_onProxRemoved(this,funcToCallback);
};



Object.defineProperty(system.__presence_constructor__.prototype, "position",
                      {
                          get: function() { return this.getPosition(); },
                          set: function() { return this.setPosition.apply(this, arguments); },
                          enumerable: true
                      }
);

Object.defineProperty(system.__presence_constructor__.prototype, "velocity",
                      {
                          get: function() { return this.getVelocity(); },
                          set: function() { return this.setVelocity.apply(this, arguments); },
                          enumerable: true
                      }
);

Object.defineProperty(system.__presence_constructor__.prototype, "orientation",
                      {
                          get: function() { return this.getOrientation(); },
                          set: function() { return this.setOrientation.apply(this, arguments); },
                          enumerable: true
                      }
);


Object.defineProperty(system.__presence_constructor__.prototype, "orientationVel",
                      {
                          get: function() { return this.getOrientationVel(); },
                          set: function() { return this.setOrientationVel.apply(this, arguments); },
                          enumerable: true
                      }
);


Object.defineProperty(system.__presence_constructor__.prototype, "scale",
                      {
                          get: function() { return this.getScale(); },
                          set: function() { return this.setScale.apply(this, arguments); },
                          enumerable: true
                      }
);


Object.defineProperty(system.__presence_constructor__.prototype, "mesh",
                      {
                          get: function() { return this.getMesh(); },
                          set: function() { return this.setMesh.apply(this, arguments); },
                          enumerable: true
                      }
);

Object.defineProperty(system.__presence_constructor__.prototype, "physics",
                      {
                          get: function() { return this.getPhysics(); },
                          set: function() { return this.setPhysics.apply(this, arguments); },
                          enumerable: true
                      }
);

system.__presence_constructor__.prototype.__prettyPrintFieldsData__ = [
    "position", "velocity",
    "orientation", "orientationVel",
    "scale", "mesh", "physics"
];
system.__presence_constructor__.prototype.__prettyPrintFields__ = function() {
    return this.__prettyPrintFieldsData__;
};

{
  /** @namespace presence */
  var presence = function()
  {
    /**@function 
       @description Returns the position of the presence
       @return the vector corresponding to the position of the presence 
       @type util.Vec3
    */
    presence.prototype.getPosition = function(){} 
   
    /**@function
       @description sets the position of the presence to a new value
       @param newpos  The new position of the presence to set to

    */

    presence.prototype.setPosition = function(/** util.Vec3 */ newpos){}
    
    /**@function
       @description sets the velocity of the presence to a new value
       @param newvel  The new velocity of the presence to set to

    */

    presence.prototype.setVelocity = function(/** util.Vec3 */ newvel){}
    
    /**@function 
       @description Returns the velocity of the presence
       @return the vector corresponding to the velocity of the presence 
       @type util.Vec3
    */

    presence.prototype.getVelocity = function(){}

    /**@function
       @description sets the orientation of the presence to a new value
       @param newvel  The new orientation of the presence to set to
    */

    presence.prototype.setOrientation = function(/** util.Quaternion */ newpos){}


    /**@function 
       @description Returns the orientation of the presence
       @return the quaternion corresponding to the  of the presence 
       @type util.Quaternion
    */

    presence.prototype.getOrientation = function(){}

    /**@function
       @description sets the orientation velocity (both angular velocity and the axis of rotation) 
          of the presence to a new value
       @param newvel  The new orientation velocity of the presence to set to
    */

    presence.prototype.setOrientationVel = function(/** util.Quaternion */ newvel){}
    
    /**@function 
       @description Returns the orientation velocity of the presence
       @return the quaternion corresponding to the  orientation velocity of the presence 
       @type util.Quaternion
    */
    
    presence.prototype.getOrientationVel = function(){}


    /**@function
       @description scales of the mesh of the presence to a new value
       @param scale the factor by which to scale the mesh 
    */

    presence.prototype.setScale = function(/**Number */ scale){}
    
    /**@function 
       @description Returns the factor by which the mesh has been scaled from the original size
       @return  the factor by which the mesh has been scaled from the original size
       @type Number 
    */

    presence.prototype.getScale = function(){}

    /**@function
       @description sets the mesh of the presence 
       @param newmesh The url for the mesh to set to
    */

    presence.prototype.setMesh = function(/**String */ newmesh){}

     /**@function 
       @description Returns the mesh of the presence
       @return  the url for the mesh of the presence 
       @type String 
    */

    presence.prototype.getMesh = function(){}


      /** @function
       @return Returns the identifier for the space that the presence is in.
       @type String
       */
      presence.prototype.getSpaceID = function(){}

      /** @function
       @return Returns the identifier for the presence in the space that it's in.
       @type String
       */
      presence.prototype.getPresenceID = function(){}
      
      
  };


}






