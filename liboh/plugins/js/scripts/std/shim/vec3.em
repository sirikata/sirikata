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


if(util.Vec3 == undefined)
{
  /** @namespace 
    * Vec3 should be assumed to only have .x, .y, and .z. */
  util.Vec3 = new Object();
}



/** @function
 @return {string} type of this object ("vec3");
 */
util.Vec3.prototype.__getType = function()
{
    return 'vec3';
};

/** @function 
  @return negation of this vector
*/
util.Vec3.prototype.neg = function() {
    return new util.Vec3(-this.x, -this.y, -this.z);
};

/** @function 
  @param rhs the vector to add to this vector
  @return vector sum of this vector and rhs
*/
util.Vec3.prototype.add = function(rhs) {
    return new util.Vec3(this.x + rhs.x, this.y + rhs.y, this.z + rhs.z);
};

/** @function 
  @param rhs the vector to subtract from this vector
  @return vector difference of this vector and rhs (this - rhs)
*/
util.Vec3.prototype.sub = function(rhs) {
    return new util.Vec3(this.x - rhs.x, this.y - rhs.y, this.z - rhs.z);
};

/** @function 
  @param rhs the vector whose components need to be multiplied
  @return the vector with the component wise product
*/
util.Vec3.prototype.componentMultiply = function(rhs) {
    return new util.Vec3(this.x * rhs.x, this.y * rhs.y, this.z * rhs.z);
};

/** @function 
  @param  rhs 
  @return if rhs is a number, returns what scale(rhs) returns
          if rhs is a Vec3, it returns the componentMultiply(rhs)
*/
util.Vec3.prototype.mul = function(rhs) {
    if (typeof(rhs) === "number")
        return this.scale(rhs);
    return this.componentMultiply(rhs);
};

/** @function 
  @param rhs a scalar
  @return returns a vector scaled by the amount rhs
*/

util.Vec3.prototype.scale = function(rhs) {
    return new util.Vec3(this.x * rhs, this.y * rhs, this.z * rhs);
};

/** @function 
  @param rhs scalar
  @return returns a vector scaled by (1/rhs)
*/
util.Vec3.prototype.div = function(rhs) {
    return new util.Vec3(this.x / rhs, this.y / rhs, this.z / rhs);
};

/** @function 
  @param rhs a Vec3
  @return a scalar representing the dot product of this vector with rhs
*/
util.Vec3.prototype.dot = function(rhs) {
    return this.x * rhs.x + this.y * rhs.y + this.z * rhs.z;
};

/** @function 
  @param rhs a Vec3
  @return a Vec3 representing the vector product of this vector with rhs
*/
util.Vec3.prototype.cross = function(rhs) {
    return new util.Vec3(
        this.y * rhs.z - this.z * rhs.y,
        this.z * rhs.x - this.x * rhs.z,
        this.x * rhs.y - this.y * rhs.x
    );
};

/** @function 
  @param rhs a Vec3
  @return a Vec3 where each component is the minimum of the components
  of this vector and rhs 
  For example, Vec3(3, 4, 7).min( Vec3(1, 5, 6) ) returns a Vec3(1, 4, 6)
*/
util.Vec3.prototype.min = function(rhs) {
    return new util.Vec3(
        rhs.x < this.x ? rhs.x : this.x,
        rhs.y < this.y ? rhs.y : this.y,
        rhs.z < this.z ? rhs.z : this.z
    );
};

/** @function 
  @param rhs a Vec3
  @return a Vec3 where each component is the maximum of the components
  of this vector and rhs
  For example, Vec3(3, 4, 7).max( Vec3(1, 5, 6)) returns a Vec3(3, 5, 7)
*/
util.Vec3.prototype.max = function(rhs) {
    return new util.Vec3(
        rhs.x > this.x ? rhs.x : this.x,
        rhs.y > this.y ? rhs.y : this.y,
        rhs.z > this.z ? rhs.z : this.z
    );
};

/** @function 
  @return the length of this vector
*/
util.Vec3.prototype.length = function() {
    return util.sqrt( this.dot(this) );
};

/** @function 
  @return the length-squared of this vector
*/
util.Vec3.prototype.lengthSquared = function() {
    return this.dot(this);
};

/** @function 
  @return the unit Vec3 normal to this vector
*/
util.Vec3.prototype.normal = function() {
    var len = this.length();
    if (len>1e-08)
        return this.div(len);
    return this;
};

/** @function 
  @param normal The Vec3 across which to take reflection
  @return the Vec3 which is the reflection of this vector in the normal
*/
util.Vec3.prototype.reflect = function(normal) {
    return new util.Vec3(this.sub( normal.dot( this.dot(normal).scale(2.0) ) ));
};

util.Vec3.prototype.__prettyPrintFieldsData__ = ["x", "y", "z"];
util.Vec3.prototype.__prettyPrintFields__ = function() {
    return this.__prettyPrintFieldsData__;
};
