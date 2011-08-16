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
if (typeof(std.movement) === "undefined") /** @namespace*/ std.movement = {};

system.require('std/core/bind.em');
system.require('std/movement/movement.em');

(
function() {

    var ns = std.movement;

    /** @namespace
     *  An Animatable listens for requests from other objects for animation
     *  information. The animatable keeps track of the current animation 
     *  running on it and informs any newly joining cameras. When a graphical 
     *  client changes an object's animation, it also informs the animatable
     *  about the new animation. The animatable 
     *  then forwards this information to all subscribing cameras.
     *
     */
    std.movement.Animatable = function(only_self) {
        var animationRequestHandler = std.core.bind(this._handleRequest, this, only_self);
        animationRequestHandler << [{"animationRequest"::}];

        // Set up a bunch of handlers based on request name so we can
        // easily dispatch them
        this._handlers = {
            intro : std.core.bind(this._handleIntro, this),
            currentAnimationName : std.core.bind(this._handleGetCurrentAnimation, this),
            setAnimation : std.core.bind(this._handleSetAnimation, this),
        };

        this._currentAnimation = "";
        this._currentMesh = "";
        this._subscription_group = [];
    };

    std.movement.Animatable.prototype._handleRequest = function(only_self, msg, sender) {
        if (only_self && !sender.checkEqual(system.self))
            return;

        for(var f in msg) {
            var handler = this._handlers[f];
            if (handler) {
                handler(msg, sender);
            }
        }
    };

    /**
     *  Handle an introduction message from a new camera by sending it animation
     *  information. 
     */
    std.movement.Animatable.prototype._handleIntro = function(msg, sender) {
       this._subscription_group.push(sender);

       if (this._currentAnimation != "") {
          if (system.self.getMesh() == this._currentMesh) {

            this._sendAnimationInfo( sender, 0 );
          }
          else {
            /* If the mesh has changed since the last time an animation was
               started, set the current animation to "".  
            */

            this._currentAnimation = "";
          }
       }

       msg.makeReply( { } ) >> [];
    };

    /**
     *   Return the current animation running on this animatable object.
     */
    std.movement.Animatable.prototype._handleGetCurrentAnimation = function(msg, sender) {
        if (this._currentAnimation != "") {
          if (system.self.getMesh() != this._currentMesh) {
            this._currentAnimation = "";
          }
        }

        msg.makeReply( {'currentAnimation' : this._currentAnimation } ) >> [];
    };

    /**
     * Set a new animation on this animatable and inform all subscribers about it.
     */
    std.movement.Animatable.prototype._handleSetAnimation = function(msg, sender) {
        this._currentAnimation = msg.animationName;
        this._currentMesh = system.self.getMesh();

        for(var i = 0; i < this._subscription_group.length; i++) {
          if (sender.toString() != this._subscription_group[i].toString() ) {
            this._sendAnimationInfo(this._subscription_group[i], 0);
          }
        }

        msg.makeReply( { } ) >> [];
    };

    /** 
     *  Send animation information to the specified destination.
     */
    std.movement.Animatable.prototype._sendAnimationInfo = function(dest, retryAttempt) {
      if (retryAttempt == 5) {
        var idx = this._subscription_group.indexOf(dest);
        if (idx != -1) {
          this._subscription_group.splice(idx, 1);
        }
        return;
      }

      if (system.self.getMesh() != this._currentMesh) return;

      var animationMsg = { "animationInfo" : "1", "animationName" : this._currentAnimation  };

      animationMsg >> dest >> [ std.core.bind(function(){}, undefined),
                                5, 
                                std.core.bind(this._sendAnimationInfo, this, dest, retryAttempt+1) ];
    };


})();
