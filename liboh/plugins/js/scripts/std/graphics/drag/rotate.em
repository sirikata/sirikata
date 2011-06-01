/*  Sirikata
 *  rotate.em
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

system.require('std/movement/movableremote.em');
system.require('std/graphics/drag/handler.em');

/** @namespace
    RotateDragHandler responds to drag events by moving a selected object.
 */
std.graphics.RotateDragHandler = std.graphics.DragHandler.extend(
    {

        /** @memberOf std.graphics.RotateDragHandler */
        init: function(gfx) {
            this._super(gfx);
        },

        // Computes the direction from the center of the sphere to the
        // point currently hit by the mouse, or undefined if the mouse
        // is currently beyond the bounds of the sphere.
        _spherePos: function(evt) {
            var center_pos = this._dragging.getPosition();
            var scale = this._dragging.getScale();

            var look_from = this._graphics.cameraPosition();
            var look_at = this._graphics.cameraDirection(evt.x, evt.y);

            var center_to_cam = look_from.sub(center_pos);

            var a = look_at.dot(look_at);
            var b = look_at.mul(2).dot( center_to_cam );
            var c = center_to_cam.dot(center_to_cam) - scale*scale;

            var descrim = b*b - 4*a*c;
            if (descrim < 0) return undefined;
            var descrim_root = util.sqrt(descrim);

            var t1 = (-b + descrim_root) / (2*a);
            var t2 = (-b + descrim_root) / (2*a);

            var t = undefined;
            if (t1 > 0) {
                if (t2 > 0)
                    t = (t1 < t2) ? t1 : t2;
                else
                    t = t1;
            } else if (t2 > 0) {
                t = t2;
            }
            if (!t) return undefined;
            var on_sphere = look_from.add(look_at.mul(t));
            return on_sphere.sub(center_pos).normal();
        },

        /** @memberOf std.graphics.RotateDragHandler */
        selected: function(obj, hitpoint, evt) {
            this._dragging = obj ?
                new std.movement.MovableRemote(obj) : null;
            this._startingDir = this._spherePos(evt);
        },

        /** @memberOf std.graphics.RotateDragHandler */
        onMouseDrag: function(evt) {
            if (!this._dragging) return;

            if (!this._dragging.dragOrientation)
                this._dragging.dragOrientation = this._dragging.getOrientation();

            var endingDir = this._spherePos(evt);
            if (!this._startingDir || !endingDir) return;

            var around = endingDir.cross(this._startingDir);
            var angle = util.asin(around.length());

            var dragRotation = (new util.Quaternion(around, angle)).mul(this._dragging.dragOrientation);
            this._dragging.setOrientation( dragRotation );
        },

        /** @memberOf std.graphics.RotateDragHandler */
        onMouseRelease: function(evt) {
            if (this._dragging)
                this._dragging.dragOrientation = null;
            this._lastClickAxis = null;
        }
    }
);
