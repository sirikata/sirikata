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

system.import('std/movement/movableremote.em');
system.import('std/graphics/drag/handler.em');

/** RotateDragHandler responds to drag events by moving a selected object.
 */
std.graphics.RotateDragHandler = std.graphics.DragHandler.extend(
    {
        init: function(gfx) {
            this._super(gfx);
        },

        selected: function(obj) {
            this._dragging = obj ?
                new std.movement.MovableRemote(obj) : null;
        },

        onMouseDrag: function(evt) {
            if (!this._dragging) return;

            if (!this._dragging.dragOrientation)
                this._dragging.dragOrientation = this._dragging.getOrientation();

            var cameraAxis = this._graphics.cameraDirection();

            var radianX = 0, radianY = 0, radianZ = 0;
            var sensitivity =0.25;

            var ctrlX = true, ctrlZ = true;
            if (ctrlX) {
                if (ctrlZ)
                    radianZ = 3.14159 * 2 * -evt.dx * sensitivity;
                else
                    if (cameraAxis.z > 0) sensitivity *=-1;
                radianX = 3.14159 * 2 * -evt.dy * sensitivity;
            }
            else {
                if (ctrlZ) {
                    if (cameraAxis.x <= 0) sensitivity *=-1;
                    radianZ = 3.14159 * 2 * -evt.dy * sensitivity;
                }
                else
                    radianY = 3.14159 * 2 * evt.dx * sensitivity;
            }
            var rotX = new util.Quaternion( new util.Vec3(1,0,0), radianX);
            var rotY = new util.Quaternion( new util.Vec3(0,1,0), radianY);
            var rotZ = new util.Quaternion( new util.Vec3(0,0,1), radianZ);
            var dragRotation = rotX.mul(rotY).mul(rotZ).mul(this._dragging.dragOrientation);
            this._dragging.setOrientation( dragRotation );
        },

        onMouseRelease: function(evt) {
            if (this._dragging)
                this._dragging.dragOrientation = null;
            this._lastClickAxis = null;
        }
    }
);
