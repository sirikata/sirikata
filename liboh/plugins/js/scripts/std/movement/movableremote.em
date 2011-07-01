/*  Sirikata
 *  movableremote.em
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

system.require('std/core/bind.em');

(
function() {

    var ns = std.movement;

    /** @namespace
     *  A MovableRemote wraps a remote object (visible) and allows you
     *  to control its movement with simple commands, assuming it is
     *  both capable and allows you to.
     */
    std.movement.MovableRemote = function(remote) {
        this._remote = remote;
        // To avoid out of order reception problems (i.e. lots of
        // messagse go out, they get out of order, earlier ones are
        // applied later), we follow a strict ping-pong, only sending
        // updates if the last one has been acked.
        this._clearToSend = true;
        // And because of this, we need to queue updates.
        this._updates = {};

        this._thisHandleAck = std.core.bind(this._handleAck, this);
    };

    std.movement.MovableRemote.prototype._sendUpdate = function(pos) {
        if (this._clearToSend) {
            this._clearToSend = false;
            this._updates.request = 'movable';
            this._updates >> this._remote >> [this._thisHandleAck, 10, this._thisHandleAck];
            this._updates = {};
        }
    };

    std.movement.MovableRemote.prototype._handleAck = function() {
        this._clearToSend = true;
        if (Object.getOwnPropertyNames(this._updates).length > 0)
            this._sendUpdate();
    };

    /** @function
      * Get the position of the visible
      @return Vec3 object
    */
    std.movement.MovableRemote.prototype.getPosition = function() {
        return this._remote.getPosition();
    };
    /** @function
      * Get the velocity of the visible
      @return Vec3 object
    */
    std.movement.MovableRemote.prototype.getVelocity = function() {
        return this._remote.getVelocity();
    };
    /** @function
      * Get orientation of the visible
    */
    std.movement.MovableRemote.prototype.getOrientation = function() {
        return this._remote.getOrientation();
    };
    /** @function */
    std.movement.MovableRemote.prototype.getOrientationVel = function() {
        return this._remote.getOrientationVel();
    };
    /** @function */
    std.movement.MovableRemote.prototype.getScale = function() {
        return this._remote.getScale();
    };
    /** @function */
    std.movement.MovableRemote.prototype.getMesh = function() {
        return this._remote.getMesh();
    };

    /** @function */
    std.movement.MovableRemote.prototype.setPosition = function(pos) {
        this._updates.position = pos;
        this._sendUpdate();
    };

    /** @function */
    std.movement.MovableRemote.prototype.move = function(dir) {
        this._updates.velocity = dir;
        this._sendUpdate();
    };

    /** @function */
    std.movement.MovableRemote.prototype.setOrientation = function(orient) {
        this._updates.orient = orient;
        this._sendUpdate();
    };

    /** @function */
    std.movement.MovableRemote.prototype.setRotationalVelocity = function(orientvel) {
        this._updates.orientvel = orientvel;
        this._sendUpdate();
    };

    /** @function */
    std.movement.MovableRemote.prototype.setScale = function(scale) {
        this._updates.scale = scale;
        this._sendUpdate();
    };

    /** @function */
    std.movement.MovableRemote.prototype.setPhysics = function(phy) {
        this._updates.physics = phy;
        this._sendUpdate();
    };

    /** @function */
    std.movement.MovableRemote.prototype.setMesh = function(msh) {
        this._updates.mesh = msh;
        this._sendUpdate();
    };

    /** @function */
    std.movement.MovableRemote.prototype.stop = function() {
        this._updates.stop = true;
        this._sendUpdate();
    };

})();
