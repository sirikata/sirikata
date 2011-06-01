/*  Sirikata
 *  move.em
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
    MoveDragHandler responds to drag events by moving a selected object.
 */
std.graphics.MoveDragHandler = std.graphics.DragHandler.extend(
    {
        /** @memberOf std.graphics.MoveDragHandler */
        init: function(gfx) {
            this._super(gfx);
        },

        /** @memberOf std.graphics.MoveDragHandler */
        selected: function(obj, hitpoint, evt) {
            if (obj) {
                this._dragging = new std.movement.MovableRemote(obj);
                this._dragPoint = hitpoint;
            }
            else {
                this._dragging = null;
                this._dragPoint = null;
            }
        },

        /** @memberOf std.graphics.MoveDragHandler */
        onMouseDrag: function(evt) {
            if (!this._dragging) return;

            if (!this._dragging.dragPosition)
                this._dragging.dragPosition = this._dragging.getPosition();

            var centerAxis = this._graphics.cameraDirection();
            var clickAxis = this._graphics.cameraDirection(evt.x, evt.y);

            var lastClickAxis = this._lastClickAxis;
            this._lastClickAxis = clickAxis;

            if (!lastClickAxis) return;

            var moveVector = this._dragPoint.sub( this._graphics.presence.getPosition() );
            var moveDistance = moveVector.dot(centerAxis);
            var start = lastClickAxis.scale(moveDistance);
            var end = clickAxis.scale(moveDistance);
            var toMove = end.sub(start);
            this._dragging.dragPosition = this._dragging.dragPosition.add(toMove);
            this._dragPoint = this._dragPoint.add(toMove);
            this._dragging.setPosition(this._dragging.dragPosition);
        },

        /** @memberOf std.graphics.MoveDragHandler */
        onMouseRelease: function(evt) {
            if (this._dragging)
                this._dragging.dragPosition = null;
            this._lastClickAxis = null;
        }
    }
);
