/*  Sirikata
 *  movementAndRotate.em
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
        UPDATE_TYPE_MOVEMENT: 1,
        UPDATE_TYPE_ROTATION: 2,
        UPDATE_TYPE_ALL: 3,

        /** Initialize the controller with a presence. The optional parameter
         *  update_cb is a function which is invoked periodically when the
         *  position or orientation are updated. This is useful if you derive
         *  some other information (e.g. camera position) from this state. The
         *  final parameter controls what kind of updates you get callbacks for:
         *  'all', 'rotation', or 'movement'. It defaults to 'all'.
         */
        init: function(pres, update_cb, update_type) {
            this._pres = pres;
            this._updatecb = update_cb; // May be undefined
            if (update_type == 'movement')
                this._update_type = this.UPDATE_TYPE_MOVEMENT;
            else if (update_type == 'rotation')
                this._update_type = this.UPDATE_TYPE_ROTATION;
            else
                this._update_type = this.UPDATE_TYPE_ALL;

            this._localVel = pres.velocity;
            this._localOrientVel = pres.orientationVel;

            this._moving = this.moving();
            this._rotating = this.rotating();
        },
        moving: function() {
            return this._pres.velocity.lengthSquared() > 1e-08;
        },
        rotating: function() {
            return !this._pres.orientationVel.isZero();
        },
        move: function(vel, scaling, overwrite) {
            var scaled_vel = vel.mul(scaling);
            if (overwrite) {
                this._localVel = scaled_vel;
                this._pres.velocity = this._pres.orientation.mul(scaled_vel);
            }else {
                this._localVel = this._localVel.add(scaled_vel);
                this._pres.velocity = this._pres.velocity.add(
                    this._pres.orientation.mul(scaled_vel)
                );
            }
            this._startReeval(true);
        },
        rotate: function(axis, angle) {
            var newVel = this._localOrientVel.mul(
                new util.Quaternion(axis, angle)
            );
            this._pres.orientationVel = newVel;
            this._localOrientVel = newVel;
            this._startReeval(true);
        },
        _startReeval: function(is_first) {
            this._moving = this.moving();
            this._rotating = this.rotating();

            // Usually people expect that holding down forward and left/right
            // will result in moving in a curve, i.e. that the velocity is in
            // local coordinates and gets updated automatically with rotation.
            // Further, we also provide callbacks if requested, so we may need
            // to setup timeouts for those, even if we don't need to update the
            // info.

            // These various conditions mean we can trigger reevaluation due to
            // a number of conditions.
            if ((this._update_type == this.UPDATE_TYPE_ALL && (this._moving || this._rotating)) ||
                (this._update_type == this.UPDATE_TYPE_MOVEMENT && this._moving) ||
                (this._update_type == this.UPDATE_TYPE_ROTATION && this._rotating) ||
                (this._moving && this._rotating || is_first) // Real reeval
               ) {
                   // is_first tracks whether this is the first
                   // request for reeval. We do this specially to
                   // force reevaluation immediately upon setting
                   // values. We need to do this because when the
                   // orientation changes we want to make sure the
                   // velocity also gets the corresponding
                   // update. Otherwise, the deltas used can become
                   // incorrect so a sequence like [ move forward,
                   // rotate right, rotate_left, move backward ],
                   // which should have all elements cancel out, can
                   // end up with a small velocity because the move
                   // backwards is computed using the right local
                   // orientation but the presence's velocity might
                   // not have been updated yet (still waiting for
                   // first timeout).
                   if (is_first)
                       this._reeval(is_first);
                   else
                       system.timeout(.05, std.core.bind(this._reeval, this));
               }
        },
        _reeval: function(is_first) {
            // Only perform reeval if we really need it
            if (this._moving && this._rotating || is_first)
                this._pres.velocity = this._pres.orientation.mul(this._localVel);

            // Then do callbacks
            if (this._updatecb &&
                (this._update_type == this.UPDATE_TYPE_ALL ||
                 (this._update_type == this.UPDATE_TYPE_MOVEMENT && this._moving) ||
                 (this._update_type == this.UPDATE_TYPE_ROTATION && this._rotating))) {
                this._updatecb();
            }

            // Loop, if necessary.
            this._startReeval();
        }
    }
);
