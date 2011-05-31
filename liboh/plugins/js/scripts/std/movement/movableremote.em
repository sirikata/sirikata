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
    std.movement.MovableRemote.prototype.setPosition = function(pos) {
        {
            request : 'movable',
            action : 'setPosition',
            position : pos
        } -> this._remote;
    };
    /** @function */
    std.movement.MovableRemote.prototype.move = function(dir) {
        {
            request : 'movable',
            action : 'setVelocity',
            velocity : dir
        } -> this._remote;
    };

    /** @function */
    std.movement.MovableRemote.prototype.setOrientation = function(orient) {
        {
            request : 'movable',
            action : 'setOrientation',
            orient : orient
        } -> this._remote;
    };

    /** @function */
    std.movement.MovableRemote.prototype.setRotationalVelocity = function(orientvel) {
        {
            request : 'movable',
            action : 'setRotationalVelocity',
            orientvel : orientvel
        } -> this._remote;
    };
    /** @function */
    std.movement.MovableRemote.prototype.setScale = function(scale) {
        {
            request : 'movable',
            action : 'setScale',
            scale : scale
        } -> this._remote;
    };

    /** @function */
    std.movement.MovableRemote.prototype.setPhysics = function(phy) {
        {
            request : 'movable',
            action : 'setPhysics',
            physics : phy
        } -> this._remote;
    };
    
    /** @function */
    std.movement.MovableRemote.prototype.stop = function() {
        {
            request : 'movable',
            action : 'stop'
        } -> this._remote;
    };

})();
