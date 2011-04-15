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

system.require('movement.em');

/** MoveAndRotate gives you intuitive control over moving and
 *  rotating. The most common use for this is when you want to combine
 *  movement and rotation simultaneously, but prefer to specify
 *  velocity post-rotation. For example, if you want to specify that
 *  an object continues to move 'forward' by specifying a single
 *  velocity, but also have the object continue even as rotation
 *  occurs.
 */
std.movement.MoveAndRotate = system.Class.extend(
    {
        init: function(pres) {
            this._pres = pres;
            this._localVel = pres.velocity;
        },
        moving: function() {
            return this._pres.velocity.lengthSquared() > 1e-08;
        },
        rotating: function() {
            return this._pres.orientationVel.lengthSquared() > 1e-08;
        },
        move: function(vel, scaling) {
            var scaled_vel = vel.mul(scaling);
            this._localVel = this._localVel.add(scaled_vel);
            this._pres.velocity = this._pres.velocity.add(
                this._pres.orientation.mul(scaled_vel)
            );
            this._startReeval();
        },
        rotate: function(axis, angle) {
            this._pres.orientationVel = this._pres.orientationVel.mul(
                new util.Quaternion(axis, angle)
            );
            this._startReeval();
        },
        _startReeval: function() {
            // Usually people expect that holding down forward and left/right
            // will result in moving in a curve, i.e. that the velocity is in
            // local coordinates and gets updated automatically with rotation.
            if (this.moving() && this.rotating())
                system.timeout(.1, std.core.bind(this._reeval, this));
        },
        _reeval: function() {
            this._pres.velocity = this._pres.orientation.mul(this._localVel);
            this._startReeval();
        }
    }
);
