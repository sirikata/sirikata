/*  Sirikata
 *  bbox.em
 *
 *  Copyright (c) 2011, Stanford University
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


/** @namespace
 *  A bounding box. */
util.BBox = function() {
    if (arguments.length == 0) {
        // BBox()
        this.min = new util.Vec3();
        this.max = new util.Vec3();
    }
    else if (arguments.length == 2 &&
             typeof(arguments[0]) === 'object' &&
             typeof(arguments[1]) === 'object') {
        // BBox(min, max)
        this.min = arguments[0];
        this.max = arguments[1];
    }
    else if (arguments.length == 1 && typeof(arguments[0]) === 'object' &&
             arguments[0].min && arguments[0].max) {
        // BBox({min: low, max: high});
        this.min = arguments[0].min;
        this.max = arguments[0].max;
    }
    else {
        throw new Error('Invalid arguments passed to util.BBox constructor.');
    }
};

/** @return {string} type of this object ("bbox");
 */
util.BBox.prototype.__getType = function()
{
    return 'bbox';
};

/** Translate the bounding box by a Vec3.
 *  @param rhs the vector to add to this vector
 *  @return translated BBox
 */
util.BBox.prototype.translate = function(rhs) {
    return new util.BBox(this.min + rhs, this.max + rhs);
};

/** Scale the bounding box by a scalar.
 *  @param rhs a scalar
 *  @return returns a bounding box scaled by the amount rhs
*/
util.BBox.prototype.scale = function(rhs) {
    return new util.BBox(this.min * rhs, this.max * rhs);
};

/** Get the size of bounding box, i.e. the vector between the min and
 *  max points.
 *  @return a Vec3 with the size of the BBox
 */
util.BBox.prototype.across = function() {
    return this.max - this.min;
};

util.BBox.prototype.__prettyPrintFieldsData__ = ["min", "max"];
util.BBox.prototype.__prettyPrintFields__ = function() {
    return this.__prettyPrintFieldsData__;
};
