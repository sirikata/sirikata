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

Object.defineProperty(system.__presence_constructor__.prototype, "query",
                      {
                          get: function() { return this.getQuery(); },
                          set: function() { return this.setQuery.apply(this, arguments); },
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

Object.defineProperty(system.__presence_constructor__.prototype, "queryCount",
                      {
                          get: function() { return this.getQueryCount(); },
                          set: function() { return this.setQueryCount.apply(this, arguments); },
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

system.__presence_constructor__.prototype.toString = function()
{
    if (typeof(this.stringified)  == 'undefined')
        this.stringified = this.__toString();
    return this.stringified;
};


{
  /** @class A presence represents your object in the world. It allows
   *  you to get and set the basic properties of the object (position,
   *  orientation, mesh, etc) as well communicate with other
   *  objects. You must have a presence to interact with the world and
   *  other objects in it. You don't create presences directly: call
   *  system.createPresence to create a new connection to the space.
   */
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


     /** Returns all data associated with this
      *  @private
      *  Returns an object with:
      *    {string} sporef,
      *    {vec3} pos,
      *    {vec3} vel,
      *    {string} posTime,
      *    {quaternion} orient,
      *    {quaternion} orientVel,
      *    {string} orientTime,
      *    {string} mesh,
      *    {number} scale,
      *    {boolean} isCleared ,
      *    {uint32} contextId,
      *    {boolean} isConnected,
      *    {function,null} connectedCallback,
      *    {boolean} isSuspended,
      *    {vec3} suspendedVelocity,
      *    {quaternion} suspendedOrientationVelocity,
      *    {float} query
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

    presence.prototype.getAnimationList = function(){}

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
       @description Sets the raw query that is issued from this presence.
       @param newQuery
    */
    presence.prototype.setQuery = function(/** String */ newQuery){};

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


    /**@function
       @description Sets the maximum number of results to be returned
            by the query issued from this presence.
       @param count the maximum number of results.
    */
    presence.prototype.setQueryCount = function(/** int */ count){};

    /**@function
       @description Returns the maximum number of results requested
       for this presence's query
    */
    presence.prototype.getQueryCount = function(){};


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

      /** Get a string representation of this presence -- a
       *  combination of the space and object identifiers which
       *  uniquely identify it.
       *  @returns {string} a unique string identifier for this
       *  presence.
       */
      presence.prototype.toString = function() {};

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

     // Models may come in with improper orientation. We provide a
     // modelOrientation setting (field + getter/setter) to adjust this
     // orientation. Most importantly, this gets transparently multiplied into
     // orientations you specify so that all motion appears 'natural' once you
     // set the proper model orientation, i.e. forward is always negative z, up
     // is always positive y, etc.

     var presence = system.__presence_constructor__;

     // Default value == identity
     presence.prototype.__modelOrientation = new util.Quaternion();

     presence.prototype.getModelOrientation = function() {
         return this.__modelOrientation;
     };
     presence.prototype.setModelOrientation = function(v) {
         // Extract orientation, making sure we get *current*
         // orientation out before updating
         var curorient = this.getOrientation();

         this.__modelOrientation = v;

         // And now we can set orientation again, which should
         // transparently multiply in our new value
         this.setOrientation(curorient);
     };

     Object.defineProperty(system.__presence_constructor__.prototype, "modelOrientation",
                           {
                               get: function() { return this.getModelOrientation(); },
                               set: function() { return this.setModelOrientation.apply(this, arguments); },
                               enumerable: true
                           }
                          );


     // Override get/setOrientation so they automatically take into account the modelOrientation.

     var orig_getOrientation = presence.prototype.getOrientation;
     var orig_setOrientation = presence.prototype.setOrientation;
     var orig_getOrientationVel = presence.prototype.getOrientationVel;
     var orig_setOrientationVel = presence.prototype.setOrientationVel;

     presence.prototype.getOrientation = function() {
         // Provide orientation without model orientation
         return orig_getOrientation.apply(this).mul(this.modelOrientation.inv());
     };

     presence.prototype.setOrientation = function(v) {
         // Multiply in additional transformation
         orig_setOrientation.apply(this, [v.mul(this.modelOrientation)]);
     };

     presence.prototype.getOrientationVel = function() {
         var oVel = orig_getOrientationVel.apply(this);
         var axis = oVel.axis();
         var speed = oVel.length();
         return new util.Quaternion(this.modelOrientation * axis, 1) * speed;
     };

     presence.prototype.setOrientationVel = function(v) {
         var axis = v.axis();
         var speed = v.length();
         orig_setOrientationVel.apply(this, [new util.Quaternion(
                                                 this.modelOrientation.inv() * axis,
                                                 1
                                             ) * speed]);
     };

})();

(function() {
     // Override settings with numbers in them to check for invalid values
     //
     // NOTE: This *must* come after the previous overrides of
     // get/setOrientation since it accepts a more flexible set of
     // arguments

     var orig_getQuery = system.__presence_constructor__.prototype.getQuery;
     var orig_setQuery = system.__presence_constructor__.prototype.setQuery;
     var orig_setPosition = system.__presence_constructor__.prototype.setPosition;
     var orig_setVelocity = system.__presence_constructor__.prototype.setVelocity;
     var orig_setOrientation = system.__presence_constructor__.prototype.setOrientation;
     var orig_setOrientationVel = system.__presence_constructor__.prototype.setOrientationVel;
     var orig_setScale = system.__presence_constructor__.prototype.setScale;

     var infiniteOrNaN = function(v) {
         return (!isFinite(v) || isNaN(v));
     };

     // FIXME we have this here currently so restore.em and system.em can use it for presence creation/restoration functions
     system.__presence_constructor__.__encodeDeprecatedQuery = function(angle, count) {
         if (typeof(angle) === 'undefined' && typeof(count) === 'undefined') return '';

         if (typeof(angle) === 'undefined' ||
             (typeof(angle) === 'number' && angle == 0))
             angle = 12.5664;

         var query = {
             'angle' : angle
         };

         if (typeof(count) !== 'undefined')
             query['max_results'] = count;

         return JSON.stringify(query);
     };

     system.__presence_constructor__.prototype.getQuery = function() {
         var string_query = orig_getQuery.apply(this);
         try {
             return JSON.parse(string_query);
         } catch (x) {
             return {};
         }
     };

     system.__presence_constructor__.prototype.setQuery = function(v) {
         if (typeof(v) === 'object')
             v = JSON.stringify(v);

         if (typeof(v) !== 'string')
             throw new TypeError('presence.setQuery expects a string');

         return orig_setQuery.apply(this, [v]);
     };

     system.__presence_constructor__.prototype.getQueryAngle = function() {
         var q = this.getQuery();
         if ('angle' in q)
             return q['angle'];
         else
             return 12.5664;
     };

     system.__presence_constructor__.prototype.setQueryAngle = function(v) {
         if (typeof(v) !== 'number')
             throw new TypeError('presence.setQueryAngle expects a number');
         if (infiniteOrNaN(v) || v < 0 || v > 12.5664)
             throw new RangeError('presence.setQueryAngle expects a value between 0 and 12.5664');

         var orig_query = this.getQuery();
         orig_query['angle'] = v;
         return this.setQuery(orig_query);
     };

     system.__presence_constructor__.prototype.getQueryCount = function() {
         var q = this.getQuery();
         if ('max_results' in q)
             return q['max_results'];
         else
             return 0;
     };

     system.__presence_constructor__.prototype.setQueryCount = function(v) {
         if (typeof(v) !== 'number')
             throw new TypeError('presence.setQueryCount expects a number');
         if (infiniteOrNaN(v) || v < 0)
             throw new RangeError('presence.setQueryCount expects a value greater than 0');

         var orig_query = this.getQuery();
         orig_query['max_results'] = v;
         return this.setQuery(orig_query);
     };

     system.__presence_constructor__.prototype.setPosition = function(v) {
         // Support a convenient shorthand for setting to the origin
         if (v === 0) return orig_setPosition.apply(this, [new util.Vec3(0,0,0)]);

         if (typeof(v) !== 'object' || typeof(v.x) !== 'number' || typeof(v.y) !== 'number' || typeof(v.z) !== 'number')
             throw new TypeError('presence.setPosition expects a Vec3 or object with x, y, and z fields that are numbers');
         if (infiniteOrNaN(v.x) || infiniteOrNaN(v.y) || infiniteOrNaN(v.z))
             throw new RangeError('presence.setPosition expects values to be finite and not NaNs');

         return orig_setPosition.apply(this, [v]);
     };

     system.__presence_constructor__.prototype.setVelocity = function(v) {
         // Support a convenient shorthand for setting to the origin
         if (v === 0) return orig_setVelocity.apply(this, [new util.Vec3(0,0,0)]);

         if (typeof(v) !== 'object' || typeof(v.x) !== 'number' || typeof(v.y) !== 'number' || typeof(v.z) !== 'number')
             throw new TypeError('presence.setVelocity expects a Vec3 or object with x, y, and z fields that are numbers');
         if (infiniteOrNaN(v.x) || infiniteOrNaN(v.y) || infiniteOrNaN(v.z))
             throw new RangeError('presence.setVelocity expects values to be finite and not NaNs');

         return orig_setVelocity.apply(this, [v]);
     };


     system.__presence_constructor__.prototype.setOrientation = function(v) {
         // Support a convenient shorthand for setting to the origin
         if (v === 0) return orig_setOrientation.apply(this, [new util.Quaternion()]);

         if (typeof(v) !== 'object' || typeof(v.x) !== 'number' || typeof(v.y) !== 'number' || typeof(v.z) !== 'number' || typeof(v.w) !== 'number')
             throw new TypeError('presence.setOrientation expects a Quaternion or object with x, y, z, and w fields that are numbers');
         if (infiniteOrNaN(v.x) || infiniteOrNaN(v.y) || infiniteOrNaN(v.z) || infiniteOrNaN(v.w))
             throw new RangeError('presence.setOrientation expects values to be finite and not NaNs');

         return orig_setOrientation.apply(this, [v]);
     };

     system.__presence_constructor__.prototype.setOrientationVel = function(v) {
         // Support a convenient shorthand for setting to the origin
         if (v === 0) return orig_setOrientationVel.apply(this, [new util.Quaternion()]);

         if (typeof(v) !== 'object' || typeof(v.x) !== 'number' || typeof(v.y) !== 'number' || typeof(v.z) !== 'number' || typeof(v.w) !== 'number')
             throw new TypeError('presence.setOrientationVel expects a Quaternion or object with x, y, z, and w fields that are numbers');
         if (infiniteOrNaN(v.x) || infiniteOrNaN(v.y) || infiniteOrNaN(v.z) || infiniteOrNaN(v.w))
             throw new RangeError('presence.setOrientationVel expects values to be finite and not NaNs');

         return orig_setOrientationVel.apply(this, [v]);
     };


     system.__presence_constructor__.prototype.setScale = function(v) {
         if (typeof(v) !== 'number')
             throw new TypeError('presence.setScale expects a number');
         if (infiniteOrNaN(v))
             throw new RangeError('presence.setScale expects value to be finite and not NaN');

         return orig_setScale.apply(this, [v]);
     };

})();

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
         system.__loadPresenceMesh(this, system.wrapCallbackForSelf(cb));
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
     system.__presence_constructor__.prototype.__raytrace = function(start, dir) {
         return __origRaytrace.apply(this, arguments);
     };

})();
