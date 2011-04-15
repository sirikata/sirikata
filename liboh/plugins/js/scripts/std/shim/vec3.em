/*  Sirikata
 *  vec3.em
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


/* Vec3 should be assumed to only have .x, .y, and .z. */

util.Vec3.prototype.neg = function() {
    return new util.Vec3(-this.x, -this.y, -this.z);
};

util.Vec3.prototype.add = function(rhs) {
    return new util.Vec3(this.x + rhs.x, this.y + rhs.y, this.z + rhs.z);
};

util.Vec3.prototype.sub = function(rhs) {
    return new util.Vec3(this.x - rhs.x, this.y - rhs.y, this.z - rhs.z);
};

util.Vec3.prototype.componentMultiply = function(rhs) {
    return new util.Vec3(this.x * rhs.x, this.y * rhs.y, this.z * rhs.z);
};

util.Vec3.prototype.mul = function(rhs) {
    if (typeof(rhs) === "number")
        return this.scale(rhs);
    return this.componentMultiply(rhs);
};

util.Vec3.prototype.scale = function(rhs) {
    return new util.Vec3(this.x * rhs, this.y * rhs, this.z * rhs);
};

util.Vec3.prototype.div = function(rhs) {
    return new util.Vec3(this.x / rhs, this.y / rhs, this.z / rhs);
};

util.Vec3.prototype.dot = function(rhs) {
    return this.x * rhs.x + this.y * rhs.y + this.z * rhs.z;
};

util.Vec3.prototype.cross = function(rhs) {
    return new util.Vec3(
        this.y * rhs.z - this.z * rhs.y,
        this.z * rhs.x - this.x * rhs.z,
        this.x * rhs.y - this.y * rhs.x
    );
};

util.Vec3.prototype.min = function(rhs) {
    return new util.Vec3(
        rhs.x < this.x ? rhs.x : this.x,
        rhs.y < this.y ? rhs.y : this.y,
        rhs.z < this.z ? rhs.z : this.z
    );
};

util.Vec3.prototype.max = function(rhs) {
    return new util.Vec3(
        rhs.x > this.x ? rhs.x : this.x,
        rhs.y > this.y ? rhs.y : this.y,
        rhs.z > this.z ? rhs.z : this.z
    );
};

util.Vec3.prototype.length = function() {
    return util.sqrt( this.dot(this) );
};

util.Vec3.prototype.lengthSquared = function() {
    return this.dot(this);
};

util.Vec3.prototype.normal = function() {
    var len = this.length();
    if (len>1e-08)
        return this.div(len);
    return this;
};

util.Vec3.prototype.reflect = function(normal) {
    return new util.Vec3(this.sub( normal.dot( this.dot(normal).scale(2.0) ) ));
};


util.Vec3.prototype.__prettyPrintFieldsData__ = ["x", "y", "z"];
util.Vec3.prototype.__prettyPrintFields__ = function() {
    return this.__prettyPrintFieldsData__;
};