
{

    // NOTE: These are just for documentation

  /** @namespace visible */
  var visible = function()
  {
      /**
       @param Vec3.
       @return Number.

       @description Returns the distance from this visible object to the position
       specified by first argument vector.
       */
      visible.prototype.dist =  function()
      {
      };

      /**
       @return A string corresponding to the URI for your current mesh.  Can pass
       this uri to setMesh functions on your own presences, but cannot set mesh
       directly on a visible.
       */
      visible.prototype.getMesh = function(){
      };


      /**
       @return Vec3 associated with the position of this visible object.

       @description Note: the returned value may be stale if the visible object is far away from you.
       */
      visible.prototype.getPosition = function(){
      };



      /**
       @return Object containing all data associated with this visible.  Fields or returned object: {string} sporef, {vec3} pos, {vec3} vel, {quaternion} orient, {quaternion} orientVel, {number} scale, {string} mesh, {string} posTime, {string} orientTime,
       */
      visible.prototype.getAllData = function()
      {
      };

      /**
       @return Number associated with the velocity at which this visible object is travelling.

       @description Note: the returned value may be stale if the visible object is far away from you.
       */
      visible.prototype.getVelocity = function(){
      };


      /** @function
       @return Returns the identifier for the space that the visible is in.
       @type String
       */
      visible.prototype.getSpaceID = function(){};

      /** @function
       @return Returns the identifier for the visible in the space that it's in.
       @type String
       */
      visible.prototype.getVisibleID = function(){};


      /**
       @return Quaternion associated with visible object's orientation.

       @description Note: the returned value may be stale if the visible object is far away from you.
       */
      visible.prototype.getOrientation = function(){
      };

      /**
       @return Angular velocity of visible object (rad/s).

       @description Note: the returned value may be stale if the visible object is far away from you.
       */
      visible.prototype.getOrientationVel = function (){
      };



      /**
       @return Number associated with how large the visible object is compared to the
       mesh it came from.
       */
      visible.prototype.getScale = function(){
      };

      /**
       @return String containing the space id and visible id for the visible.  Can use this string as an address through which can send messages.
       */
      visible.prototype.toString = function(){
      };


      /**
       @return Boolean.  If true, positions and velocities for this visible object
       are automatically being updated by the system.
       */
      visible.prototype.getStillVisible = function(){
      };

      /**
       @param Visible object.
       @return Returns true if the visible objects correspond to the same presence
       in the virtual world.
       */
      visible.prototype.checkEqual = function(){
      };
  };

}

// These are the real wrappers
(function() {

     // Hide visible but let rest of method override behavior.
     var visible = system.__visible_constructor__;
     delete system.__visible_constructor__;

     Object.defineProperty(visible.prototype, "position",
                           {
                               get: function() { return this.getPosition(); },
                               enumerable: true
                           }
                          );

     Object.defineProperty(visible.prototype, "velocity",
                           {
                               get: function() { return this.getVelocity(); },
                               enumerable: true
                           }
                          );

     Object.defineProperty(visible.prototype, "orientation",
                           {
                               get: function() { return this.getOrientation(); },
                               enumerable: true
                           }
                          );


     Object.defineProperty(visible.prototype, "orientationVel",
                           {
                               get: function() { return this.getOrientationVel(); },
                               enumerable: true
                           }
                          );


     Object.defineProperty(visible.prototype, "scale",
                           {
                               get: function() { return this.getScale(); },
                               enumerable: true
                           }
                          );


     Object.defineProperty(visible.prototype, "mesh",
                           {
                               get: function() { return this.getMesh(); },
                               enumerable: true
                           }
                          );

      var decodePhysics = function(phy) {
          if (phy.length == 0) return {};
          return JSON.parse(phy);
      };

     Object.defineProperty(visible.prototype, "physics",
                           {
                               get: function() { return decodePhysics(this.getPhysics()); },
                               enumerable: true
                           }
                          );

     visible.prototype.__prettyPrintFieldsData__ = [
         "position", "velocity",
         "orientation", "orientationVel",
         "scale", "mesh", "physics"
     ];
     visible.prototype.__prettyPrintFields__ = function() {
         return this.__prettyPrintFieldsData__;
     };

     /** @function
      @return {string} type of this object ("visible");
      */
     visible.prototype.__getType = function()
     {
         return  'visible';
     };
     

})();