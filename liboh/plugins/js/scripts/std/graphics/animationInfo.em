/*  Sirikata
 *  animationInfo.em
 *
 *  Copyright (c) 2011, Tahir Azim.
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
if (typeof(std.graphics) === "undefined") /** @namespace*/ std.graphics = {};


system.require("std/library.em");

/** The AnimationInfo class enables the graphics system to send out
    notifications when an object starts or stops animating.
 */
std.graphics.AnimationInfo = system.Class.extend(
    {
        init: function(pres, sim) {
            this._pres = pres;
            this._subscription_group = [];

            //short-circuit
            // if (sim) {
            //   this._simulator = sim;
            //   var animationRequestHandler = std.core.bind(this.onAnimationMessage, this);
            //   animationRequestHandler << [{"animationInfo"::}];
            // }

//            this._pres.onProxAdded(std.core.bind(this.proxAddedCallback, this), true);
        },


        // Handles requests to send an animation info messages.
        sendAnimationInfo: function(cmd, vis_addr, anim_name, retryAttempt) {
            if (retryAttempt == 5) return;
      
            if (!anim_name) {
              anim_name="";
            }

            if (!retryAttempt) {
              retryAttempt = 0;
            }

            var msg = { "animationRequest" : "1",  "setAnimation" : "1",  "animationName" : anim_name  };

            msg >> vis_addr >> [std.core.bind(this.handleAnimationResponse, this), 5, std.core.bind(this.sendAnimationInfo, this, cmd, vis_addr, anim_name, retryAttempt+1) ];
        },

        // Handler for animation msgs from others.
        onAnimationMessage: function(msg, sender) {
            msg.makeReply( {} ) >> [];

            if (!this._simulator) return;

            var visibleMap = system.getProxSet(system.self);
            for (var key in visibleMap) {
              if (sender == key) {
                this._simulator.startAnimation(sender, msg.animationName, true);
              }
           }
        },

        handleAnimationResponse: function(msg, sender) {
        },

        proxAddedCallback: function(new_addr_obj) {
            if(system.self.toString() == new_addr_obj.toString())
                return;

            this.sendIntro(new_addr_obj, 5);
        },

        sendIntro: function(new_addr_obj, retries) {
            if (retries == 0) return;

            //also register a callback
            var msg = { "animationRequest" : "1", "intro" : "1"  };
            msg >> new_addr_obj >> [std.core.bind(this.handleAnimationResponse, this), 5, std.core.bind(this.sendIntro, this, new_addr_obj, retries-1) ];

        },

    }
);
