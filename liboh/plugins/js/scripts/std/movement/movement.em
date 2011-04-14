/*  Sirikata
 *  movement.em
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

if (typeof(std) === "undefined") std = {};
if (typeof(std.movement) === "undefined") std.movement = {};

(
function() {

    var ns = std.movement;

    ns.position = function(pres, pos) {
        pres.setPosition(pos);
    };

    ns.move = function(pres, dir, amount) {
        var orient = pres.getOrientation();
        var vel = orient.mul(dir);
        if (amount)
            vel = vel.scale(amount);
        pres.setVelocity(vel);
    };

    ns.stopMove = function(pres) {
        ns.move(pres, new util.Vec3(0,0,0), 0);
    };

    ns.orientation = function(pres, orient) {
        pres.setOrientation(orient);
    };

    ns.rotate = function(pres) {
        if (arguments.length == 2) { // quaternion
            pres.setOrientationVel(arguments[1]);
        }
        else if (arguments.length == 3) { // axis-angle
            var about = arguments[1];
            var amount = arguments[2];
            pres.setOrientationVel(new util.Quaternion(about, amount));
        }
    };

    ns.stopRotate = function(pres) {
        ns.rotate(pres, new util.Vec3(1,0,0), 0);
    };

    ns.scaleBy = function(pres, scale) {
        pres.setScale( pres.getScale(scale) * scale );
    };

    ns.scaleTo = function(pres, scale) {
        pres.setScale(scale);
    };


})();