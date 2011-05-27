/** @namespace
  * CFrame should be assumed to only have .t (translation) and .r (rotation). */

/**
  @function
  @param pos The position of the coordinate frame.
  @param lookAt The point in world space that the coordinate frame should look at
         (the -z axis of the coordinate frame will point at this point).
  @param up (Optional) A vector in world space that represents the up direction
         of the coordinate frame. Defaults to <0, 1, 0>.
*/
util.CFrame = function(pos, lookAt, up) {
    if (typeof(lookAt.w) === 'number') {
        // lookAt is a quaternion.
        this.t = pos;
        this.r = lookAt;
        return;
    }
    up = up || new util.Vec3(0, 1, 0);
    this.t = pos;
    this.r = util.Quaternion.fromLookAt(lookAt - pos, up);
}

/**
  @function
  @return The coordinate frame created by yawing it by angle radians from the
          look system of the current quaternion. Yawing with a positive value will
          turn the coordinate frame to the right.
*/
util.CFrame.prototype.yaw = function(angle) {
    var newQuat = new util.Quaternion(this.r.yAxis(), angle);
    return new util.CFrame(this.t, newQuat.mul(this.r));
}

/**
  @function
  @return The coordinate frame created by pitching it by angle radians from the
          look system of the current quaternion. Pitching with a positive value will
          turn the coordinate frame up.
*/
util.CFrame.prototype.pitch = function(angle) {
    var newQuat = new util.Quaternion(this.r.xAxis(), angle);
    return new util.CFrame(this.t, newQuat.mul(this.r));
}

/**
  @function
  @return The coordinate frame created by pitching it by angle radians from the
          look system of the current quaternion. Rolling with a positive value will
          rotate the coordinate frame to the right, when looking down the look direction
          of the coordinate frame.
*/
util.CFrame.prototype.roll = function(angle) {
    var newQuat = new util.Quaternion(this.r.zAxis(), angle);
    return new util.CFrame(this.t, newQuat.mul(this.r));
}

/**
  @function
  @return The inverse of the coordinate frame.
*/
util.CFrame.prototype.inverse = function() {
    var quatInv = this.r.inverse();
    return new util.CFrame(-quatInv.mul(this.t), quatInv);
}

/**
  @function
  @return Transforms a vector from the coordinate frame to global world coordinates.
*/
util.CFrame.prototype.transform = function(vec) {
    return this.t + this.r.mul(vec);
}

/**
  @function
  @return The coordination frame created by moving the coordinate frame by the given translation,
          given in coordinate frame coordinates. For example, moving by <0, 0, -1> should move the
          coordinate frame forwards along its look direction.
*/
util.CFrame.prototype.move = function(translation) {
    return new util.CFrame(this.t + this.r.mul(translation), this.r);
}

/**
  @function
  @return The coordinate frame created by rotating the coordinate frame by the given quaternion.
*/
util.CFrame.prototype.rotate = function(rotation) {
    return new util.CFrame(this.t, this.r.mul(rotation));
}

/**
  @function
  @return The coordination frame created by rotating the coordinate frame around the given point
          by the given rotation.
*/
util.CFrame.prototype.rotateAroundPoint = function(rotation, point) {
    var newR = this.r.mul(rotation);
    var newT = point + rotation.mul(this.t - point);
    return new util.CFrame(newT, newR);
}

/**
  @function
  @return A vector pointing to the right from the perspective of the coordinate frame,
          in world space.
*/
util.CFrame.prototype.right = function() { return this.r.xAxis(); }

/**
  @function
  @return A vector pointing up from the perspective of the coordinate frame,
          in world space.
*/
util.CFrame.prototype.up = function() { return this.r.yAxis(); }

/**
  @function
  @return A vector pointing forwards from the perspective of the coordinate frame,
          in world space.
*/
util.CFrame.prototype.forwards = function() { return this.r.zAxis().scale(-1); }
