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

(function() {
     // Just a cache, but the underlying system will also store copies
     // and give us the existing one. This is necessary to make reset
     // transparent.
     var sims = {};

     system.__presence_constructor__.prototype.runSimulation = function(name) {
         if (!sims[this]) sims[this] = {};
         if (!sims[this][name]) sims[this][name] = this._runSimulation(name);
         return sims[this][name];
     };
 })();

system.__presence_constructor__.prototype.getSimulation =
    system.__presence_constructor__.prototype.runSimulation;



system.__presence_constructor__.prototype.onProxAdded =
function (funcToCallback,onExisting)
{
    var cb_handle = system.__sys_register_onProxAdded(this,funcToCallback,onExisting);
    // Return a callback handle with just a clear method
    return {
        'clear' : std.core.bind(this._delProxAdded, this, cb_handle)
    };
};

system.__presence_constructor__.prototype.onProxRemoved =
function (funcToCallback)
{
    var cb_handle = system.__sys_register_onProxRemoved(this,funcToCallback);
    // Return a callback handle with just a clear method
    return {
        'clear' : std.core.bind(this._delProxRemoved, this, cb_handle)
    };
};


system.__presence_constructor__.prototype._delProxAdded =
function (idToDelete)
{
    system.__sys_register_delProxAdded(this,idToDelete);
};

system.__presence_constructor__.prototype._delProxRemoved =
function (idToDelete)
{
    system.__sys_register_delProxRemoved(this,idToDelete);
};

system.__presence_constructor__.prototype.proxResultSet = {};


Object.defineProperty(system.__presence_constructor__.prototype, "position",
                      {
                          get: function() { return this.getPosition(); },
                          set: function() { return this.setPosition.apply(this, arguments); },
                          enumerable: true
                      }
);

Object.defineProperty(system.__presence_constructor__.prototype, "queryAngle",
                      {
                          get: function() { return this.getQueryAngle(); },
                          set: function() { return this.setQueryAngle.apply(this, arguments); },
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
                          set: function() { return this.updatePhysics.apply(this, arguments); },
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

system.__presence_constructor__.prototype.__getType = function()
{
  return 'presence';
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


      /**
       @function
       Disconnects presence from space.  Emerson presence object still exists, but its isConnected field
       evaluates to false.
       */
      presence.prototype.disconnect = function()
      {
      };
      

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
      @param {function,null} connectedCallback,
      @param {boolean} isSuspended,
      @param {vec3} suspendedVelocity,
      @param {quaternion} suspendedOrientationVelocity,
      @param {float} solidAngleQuery
      */
      presence.prototype.getAllData = function()
      {
      };

      
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
       @description Allows a scripter to set a callback to execute
       when visibles are added to the proximity set.  Setting
       onExisting to true, allows scripter to set a callback for any
       visibles added to the proximity set or that already exist in
       the set.
       @param {function} cb Callback function to execute on proximity events.
       @param {bool} (optional) onExisting If true, cb will execute
       over all existing visibles in result set as well as all future
       visibles added to set.
       @return {object} Can call clear on this object to de-register
       cb.
       */
      presence.prototype.onProxAdded = function(){ };

      /**@function
       @description Allws a scirpter to set a callback that executes
       when visibles are removed from the proximity set.
       @param {function} cb Callback function to execute when visible
       leaves this presence's proximity result set.
       @return {object} Can call clear on this object to de-register cb.
       */
      presence.prototype.onProxRemoved = function(){ };

    /**@function
       @description Sets the solid angle query that is issued from this presence.
       @param newQueryAngle Pinto should return this presence prox
       results for presences that are larger than newQueryAngle.
    */
    presence.prototype.setQueryAngle = function(/** float */ newQueryAngle){};

    /**@function
       @description Returns the solid angle that pinto is using to
       provide prox results for this presence.
    */
    presence.prototype.getQueryAngle = function(){};

      
      
      /** @function
       *  @description Returns the (decoded) physics settings of the presence.
       *  @type Object
       */
      presence.prototype.getPhysics = function() {};
      
      /** @function
       *  @description Sets the physics parameters for the
       *               presence. Unspecified values will use the
       *               defaults even if they were previously
       *               specified.
       *  @param newphys The new physics settings for the object
       */
      presence.prototype.setPhysics = function(/**Object */ newphys) {};

      /** @function
       *  @description Updates the physics parameters for the presence. Unspecified values use their previous setting.
       *  @param newphys The new physics settings for the object.
       */
      presence.prototype.updatePhysics = function(/**Object */ newphys) {};
      
      /** @function
       @return Returns the identifier for the space that the presence is in.
       @type String
       */
      presence.prototype.getSpaceID = function(){};

      /** @function
       @return Returns the identifier for the presence in the space that it's in.
       @type String
       */
      presence.prototype.getPresenceID = function(){};
      

      /** @function
       *  @description Requests that the mesh for this model is
       *  downloaded and parsed, making it available for operations
       *  like raytracing and getting precise bounding box
       *  information.
       *  @param {function} cb callback to invoke when loading is
       *  complete. The callback takes no parameters.
       *
       */
      presence.prototype.loadMesh = function(cb) {};
  };


}

(function() {

      // Physics is stored as an opaque string. This takes care of
      // encoding and decoding that string. Currently the only
      // supported type is JSON encoding.
      
      var decodePhysics = function(phy) {
          if (phy.length == 0) return {};
          return JSON.parse(phy);
      };
      
      var encodePhysics = function(phy) {
          return JSON.stringify(phy);
      };
      
      var valid_physics_fields = ['treatment', 'bounds', 'mass'];
      var orig_get_physics = system.__presence_constructor__.prototype.getPhysics;
      var orig_set_physics = system.__presence_constructor__.prototype.setPhysics;

      system.__presence_constructor__.prototype.getPhysics = function() {
          return decodePhysics( orig_get_physics.apply(this) );
      };

      system.__presence_constructor__.prototype.setPhysics = function(update) {
          var raw = {};
          for(var i in valid_physics_fields) {
              if (valid_physics_fields[i] in update)
                  raw[valid_physics_fields[i]] = update[valid_physics_fields[i]];
          }
          return orig_set_physics.apply(this, [encodePhysics(raw)]);
      };

      system.__presence_constructor__.prototype.updatePhysics = function(update) {
          var raw = this.getPhysics();
          for(var i in valid_physics_fields) {
              if (valid_physics_fields[i] in update)
                  raw[valid_physics_fields[i]] = update[valid_physics_fields[i]];
          }
          return orig_set_physics.apply(this, [encodePhysics(raw)]);
      };

     // FIXME we shouldn't need __origLoadMesh but getting at the
     // real, internal system object requires us to route through
     // system right now
     system.__presence_constructor__.prototype.__origLoadMesh = system.__presence_constructor__.prototype.loadMesh;
     system.__presence_constructor__.prototype.loadMesh = function(cb) {
         system.__loadPresenceMesh(this, cb);
     };
     // No need to override unloadMesh since it takes no special parameters

     var __origMeshBounds = system.__presence_constructor__.prototype.meshBounds;
     var __origUntransformedMeshBounds = system.__presence_constructor__.prototype.untransformedMeshBounds;
     var decodeBBox = function(raw) {
         return new util.BBox(raw[0], raw[1]);
     };
     system.__presence_constructor__.prototype.meshBounds = function() {
         return decodeBBox(__origMeshBounds.apply(this));
     };
     system.__presence_constructor__.prototype.untransformedMeshBounds = function() {
         return decodeBBox(__origUntransformedMeshBounds.apply(this));
     };

     // The basic raytrace is in *completely untransformed* mesh space, meaning
     // not even the translate/rotate/scale of the presence/visible is
     // included. Wrappers provide those types of raytracing.
     var __origRaytrace = system.__presence_constructor__.prototype.raytrace;
     system.__presence_constructor__.prototype.raytrace = function(start, dir) {
         return __origRaytrace.apply(this, arguments);
     };

})();
