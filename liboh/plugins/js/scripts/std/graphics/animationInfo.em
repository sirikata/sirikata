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

system.require("std/library.em");

/** The AnimationInfo class enables the graphics system to send out
    notifications when an object starts or stops animating.
 */
std.graphics.AnimationInfo = system.Class.extend(
    {
        init: function(pres, sim) {
            this._pres = pres;

            this._subscription_group = [];

            this._simulator = sim;

            var p  = new util.Pattern("name", "support_animation");
            std.core.bind(this.onTestMessage, this) << p;
           this._pres.onProxAdded(std.core.bind(this.proxAddedCallback, this), true);
        },


        // Send a message to all current members of the chat group
        sendAll: function(msg) {
            for(var i = 0; i < this._subscription_group.length; i++) {
                system.__debugPrint("Sending anim msg to " + this._subscription_group[i] + " \n");
                msg >> this._subscription_group[i] >> [];
            }
        },

        // Handles requests from the UI to send a chat messages.
        sendAnimationInfo: function(cmd, vis_addr, anim_name) {
            if (cmd == 'AnimationInfo' && vis_addr && anim_name)
                this.sendAll( { 'animation_info' : '1', 'vis_addr' : vis_addr, 'anim_name' : anim_name } );
        },

        // Handler for animation msgs from others.
        onAnimationMessage: function(msg, sender) {
            system.__debugPrint("Received anim msg: " + msg.vis_addr + " : " + msg.anim_name + "\n");
          
            var visibleMap = system.getProxSet(system.self);
            for (var key in visibleMap) {
              system.__debugPrint(key + " compared to "  + msg.vis_addr + "\n");
              if (msg.vis_addr == key) {
                this._simulator.startAnimation(visibleMap[key], msg.anim_name, true);
              }
           }
        },

        // Handle an initial message from a new neighbor, adding them and listening for messages from them.
        handleNewAnimationSubscriber: function(msg, sender) {
            if (msg.support_animation != 'yes') return;

            for(var i = 0; i < this._subscription_group.length; i++) {
                if(this._subscription_group[i].toString() == sender.toString())
                    return;
            }

            system.__debugPrint("Adding "  + sender + " to subscription group\n");

            this._subscription_group.push(sender);
            var p = new util.Pattern("animation_info");
            std.core.bind(this.onAnimationMessage, this) << p << sender;
        },

        proxAddedCallback: function(new_addr_obj) {
            if(system.self.toString() == new_addr_obj.toString())
                return;

            this.sendIntro(new_addr_obj, 5);
        },

        sendIntro: function(new_addr_obj, retries) {
            if (retries == 0) return;

            var test_msg = { "name" : "support_animation" };
            //also register a callback
            test_msg >> new_addr_obj >> [std.core.bind(this.handleNewAnimationSubscriber, this), 2, std.core.bind(this.sendIntro, this, new_addr_obj, retries-1)];
        },

        // Reply to probes for what protocols we support.
        onTestMessage: function(msg, sender) {
            system.__debugPrint("Replying to intro msg\n");
            msg.makeReply( { "support_animation": "yes" } ) >> [];
        }
    }
);
