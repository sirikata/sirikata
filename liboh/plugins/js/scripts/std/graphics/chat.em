/*  Sirikata
 *  chat.em
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

system.require("std/library.em");
system.require('std/escape.em');

/** The chat class enables chat with other nearby objects. It brings up a UI for
 *  chatting and deals with sending and receiving chat messages.
 */
std.graphics.Chat = system.Class.extend(
    {
        init: function(pres, sim) {
            this._pres = pres;
            this._sim = sim;

            this._chat_group = [];

            this._ui = this._sim.addGUIModule("chat_dialog", "chat/chat.js");
            this._ui.bind("event", std.core.bind(this.onSendChat, this));
            var p  = new util.Pattern("name", "get_protocol");
            std.core.bind(this.onTestMessage, this) <- p;
            this._pres.onProxAdded(std.core.bind(this.proxAddedCallback, this));
        },

        toggle: function() {
            this._ui.eval('Chat.toggleVisible()');
        },

        // Send a message to all current members of the chat group
        sendAll: function(msg) {
            for(var i = 0; i < this._chat_group.length; i++) {
                msg -> this._chat_group[i];
            }
        },

        // Handles requests from the UI to send a chat messages.
        onSendChat: function(cmd, username, msg) {
            if (cmd == 'Chat' && msg)
                this.sendAll( { username : username, chat : msg } );
        },

        // Handle a chat message from someone else.
        onChatFromNeighbor: function(msg, sender) {
            this._ui.eval('Chat.addMessage(' +  Escape.escapeString(msg.username + ': ' + msg.chat, '"') + ')' );
        },

        // Handle an initial message from a new neighbor, adding them and listening for messages from them.
        handleNewChatNeighbor: function(msg, sender) {
            for(var i = 0; i < this._chat_group.length; i++) {
                if(this._chat_group[i].toString() == sender.toString())
                    return;
            }

            this._chat_group.push(sender);
            var p = new util.Pattern("chat");
            std.core.bind(this.onChatFromNeighbor, this) <- p <- sender;
        },

        // Handler for prox events so we can check for other chat clients.
        proxAddedCallback: function(new_addr_obj) {
            if(system.self.toString() == new_addr_obj.toString())
                return;

            var test_msg = { name : "get_protocol" };

            //also register a callback
            var p = new util.Pattern("protocol", "chat");
            std.core.bind(this.handleNewChatNeighbor, this) <- p <- new_addr_obj;
            test_msg -> new_addr_obj;
        },

        // Reply to probes for what protocols we support.
        onTestMessage: function(msg, sender) {
            var reply = { "protocol": "chat" };
            reply -> sender;
        }

    }
);
