/*  Sirikata
 *  quaternion.em
 *
 *  Copyright (c) 2011, Ewen Cheslack-Postava
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


/* Quaternion should be assumed to only have .x, .y, .z, and .w. */

util.Quaternion.prototype.add = function(rhs) {
    return new util.Quaternion(this.x + rhs.x, this.y + rhs.y, this.z + rhs.z, this.w + rhs.w);
};

util.Quaternion.prototype.sub = function(rhs) {
    return new util.Quaternion(this.x - rhs.x, this.y - rhs.y, this.z - rhs.z, this.w - rhs.w);
};

util.Quaternion.prototype.neg = function() {
    return new util.Quaternion(-this.x, -this.y, -this.z, -this.w);
};

util.Quaternion.prototype.dot = function(rhs) {
    return this.x * rhs.x + this.y * rhs.y + this.z * rhs.z + this.w * rhs.w;
};

util.Quaternion.prototype.mul = function(rhs) {
    if (typeof(rhs) === "number") // scalar
        return this.scale(rhs);
    else if (
        typeof(rhs.x) === 'number' &&
            typeof(rhs.y) === 'number' &&
            typeof(rhs.z) === 'number')
    {
        if (typeof(rhs.w) === 'number') {
            // Quaternion
            return new util.Quaternion(
                this.w*rhs.x + this.x*rhs.w + this.y*rhs.z - this.z*rhs.y,
                this.w*rhs.y + this.y*rhs.w + this.z*rhs.x - this.x*rhs.z,
                this.w*rhs.z + this.z*rhs.w + this.x*rhs.y - this.y*rhs.x,
                this.w*rhs.w - this.x*rhs.x - this.y*rhs.y - this.z*rhs.z
            );
        }
        else { // Vec3
            var quat_axis = new util.Vec3(this.x, this.y, this.z);
            var uv = quat_axis.cross(rhs);
            var uuv= quat_axis.cross(uv);
            uv = uv.scale(2.0 * this.w);
            uuv = uuv.scale(2.0);
            return rhs.add(uv).add(uuv);
        }
    }
    else
        throw new TypeError('Quaternion.mul parameter must be numeric, Vec3, or Quaternion.');
};

util.Quaternion.prototype.scale = function(rhs) {
    return new util.Quaternion(this.x*rhs, this.y*rhs, this.z*rhs, this.w*rhs);
};

util.Quaternion.prototype.length = function() {
    return util.sqrt( this.dot(this) );
};

util.Quaternion.prototype.lengthSquared = function() {
    return this.dot(this);
};

util.Quaternion.prototype.normal = function() {
    var len = this.length();
    if (len>1e-08)
        return this.scale(1.0/len);
    return this;
};

util.Quaternion.prototype.inverse = function() {
    var len = this.lengthSquared();
    if (len>1e-8)
        return new util.Quaternion(-this.x/len,-this.y/len,-this.z/len,this.w/len);
    return new util.Quaternion(0.0, 0.0, 0.0, 0.0);
};
util.Quaternion.prototype.inv = util.Quaternion.prototype.inverse;

util.Quaternion.prototype.xAxis = function() {
    var fTy  = 2.0*this.y;
    var fTz  = 2.0*this.z;
    var fTwy = fTy*this.w;
    var fTwz = fTz*this.w;
    var fTxy = fTy*this.x;
    var fTxz = fTz*this.x;
    var fTyy = fTy*this.y;
    var fTzz = fTz*this.z;

    return new util.Vec3(1.0-(fTyy+fTzz), fTxy+fTwz, fTxz-fTwy);
};

util.Quaternion.prototype.yAxis = function() {
    var fTx  = 2.0*this.x;
    var fTy  = 2.0*this.y;
    var fTz  = 2.0*this.z;
    var fTwx = fTx*this.w;
    var fTwz = fTz*this.w;
    var fTxx = fTx*this.x;
    var fTxy = fTy*this.x;
    var fTyz = fTz*this.y;
    var fTzz = fTz*this.z;

    return new util.Vec3(fTxy-fTwz, 1.0-(fTxx+fTzz), fTyz+fTwx);
};

util.Quaternion.prototype.zAxis = function() {
    var fTx  = 2.0*this.x;
    var fTy  = 2.0*this.y;
    var fTz  = 2.0*this.z;
    var fTwx = fTx*this.w;
    var fTwy = fTy*this.w;
    var fTxx = fTx*this.x;
    var fTxz = fTz*this.x;
    var fTyy = fTy*this.y;
    var fTyz = fTz*this.y;
    return new util.Vec3(fTxz+fTwy, fTyz-fTwx, 1.0-(fTxx+fTyy));
};

util.Quaternion.prototype.__prettyPrintFieldsData__ = ["x", "y", "z", "w"];
util.Quaternion.prototype.__prettyPrintFields__ = function() {
    return this.__prettyPrintFieldsData__;
};
