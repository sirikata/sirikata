/*  Sirikata
 *  movable.em
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

system.import('std/core/bind.js');
system.import('std/movement/movement.em');

(
function() {

    var ns = std.movement;

    /** A Movable listens for requests from other objects to move and
     *  applies them to itself. This allows objects to control each
     *  other, e.g. for a client to move objects in the world.
     */
    ns.Movable = function(pres) {
        this._pres = pres;

        var moveRequestHandler = std.core.bind(this._handleRequest, this);
        moveRequestHandler <- new util.Pattern("request", "movable");

        // Set up a bunch of handlers based on request name so we can
        // easily dispatch them
        this._handlers = {
            stopMove : std.core.bind(this._handleStopMove, this),
            stopRotate : std.core.bind(this._handleStopRotate, this),
            stop : std.core.bind(this._handleStop, this),
            setPosition : std.core.bind(this._handleSetPos, this),
            setVelocity : std.core.bind(this._handleSetVel, this),
            setOrientation : std.core.bind(this._handleSetRot, this),
            setRotationalVelocity : std.core.bind(this._handleSetRotVel, this),
            setScale : std.core.bind(this._handleSetScale, this)
        };
    };

    ns.Movable.prototype._handleRequest = function(msg, sender) {
        var handler = this._handlers[msg.action];
        if (handler)
            handler(msg, sender);
    };

    ns.Movable.prototype._handleStopMove = function(msg, sender) {
        std.movement.stopMove(this._pres);
    };

    ns.Movable.prototype._handleStopRotate = function(msg, sender) {
        std.movement.stopRotate(this._pres);
    };

    ns.Movable.prototype._handleStop = function(msg, sender) {
        std.movement.stopMove(this._pres);
        std.movement.stopRotate(this._pres);
    };

    ns.Movable.prototype._handleSetPos = function(msg, sender) {
        std.movement.position(this._pres, msg.position);
    };

    ns.Movable.prototype._handleSetVel = function(msg, sender) {
        std.movement.move(this._pres, msg.velocity);
    };

    ns.Movable.prototype._handleSetRot = function(msg, sender) {
        std.movement.orientation(this._pres, msg.orient);
    };

    ns.Movable.prototype._handleSetRotVel = function(msg, sender) {
        std.movement.rotate(this._pres, msg.orientvel);
    };

    ns.Movable.prototype._handleSetScale = function(msg, sender) {
        std.movement.scaleTo(this._pres, msg.scale);
    };

})();
