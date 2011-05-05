/*  Sirikata
 *  inputbinding.em
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


(function() {

     var getToggleValue = function(evt, onval, offval) {
         if (evt.msg == 'button-pressed' || evt.msg == 'mouse-press') {
             return onval;
         }
         else if (evt.msg == 'button-up' || evt.msg == 'mouse-release') {
             return offval;
         }
         else {
             return undefined;
         }
     };
     var wrapToggleAction = function(cb, onval, offval) {
         return function(evt) {
             var val = getToggleValue(evt, onval, offval);
             if (val === undefined) return;
             cb(val);
         };
     };

     var getFloat2Value = function(evt) {
         if (evt.x && evt.y)
             return [evt.x, evt.y];
         return undefined;
     };
     var wrapFloat2Action = function(cb) {
         return function(evt) {
             var val = getFloat2Value(evt);
             if (val === undefined) return;
             cb(val[0], val[1]);
         };
     };

     /** @namespace
      *  InputBinding manages mapping input events to simple actions.
      */
     std.graphics.InputBinding = system.Class.extend(
         {
             /** @memberOf std.graphics.InputBinding */
             init: function() {
                 this.actions = {};
                 this.bindings = {};

                 // These types are used to register a collection of
                 // bindings, e.g. to get all key events sent to a
                 // handler instead of one specific one. This is
                 // useful, e.g., for toggles or drag handlers that
                 // need to deal with multiple events.
                 this.binding_types = {
                     'button' : ['button-pressed', 'button-repeat', 'button-up', 'button-down'],
                     'mouse' : ['mouse-hover', 'mouse-press', 'mouse-release', 'mouse-click', 'mouse-drag']
                 };

             },

             /** @memberOf std.graphics.InputBinding
              *  Add an input binding for trigger, causing the action named by act to be invoked.
              */
             addBinding: function(trigger, act) {
                 trigger = {
                     type : trigger[0],
                     key : trigger[1],
                     modifiers : trigger[2]
                 };

                 // Map type to list of types, or to list of just itself if it is already a single type
                 if (trigger.type in this.binding_types)
                     trigger.type = this.binding_types[trigger.type];
                 else
                     trigger.type = [trigger.type];

                 // Map modifiers to the smallest subset of options
                 if (trigger.modifiers === undefined) {
                     trigger.modifiers = 'none';
                 }
                 else if(trigger.modifiers === '*') {
                     trigger.modifiers = 'any';
                 }

                 for(var typeidx in trigger.type) {
                     var type = trigger.type[typeidx];
                     if (!this.bindings[type]) this.bindings[type] = {};
                     var type_binding = this.bindings[type];

                     if (!type_binding[trigger.key]) type_binding[trigger.key] = {};
                     var key_binding = type_binding[trigger.key];

                     if (!key_binding[trigger.modifiers]) key_binding[trigger.modifiers] = [];
                     key_binding[trigger.modifiers].push(act);
                 }
             },

             /** @memberOf std.graphics.InputBinding
              *  Add a set of bindings stored in an array.
              */
             addBindings: function(bindings) {
                 for(var idx in bindings)
                     this.addBinding(bindings[idx].key, bindings[idx].action);
             },

             /** @memberOf std.graphics.InputBinding
              *
              *  Adds an action to the InputBinding, associated with the
              *  given name. This is the most basic form -- it won't
              *  trigger any transformation of the event. Use this for
              *  trivial actions, i.e. those that just need to know the
              *  event occurred, or actions which require complicated
              *  transformations that are not built in (and therefore need
              *  the entire event object).
              */
             addAction: function(name, action) {
                 this.actions[name] = action;
             },

             /** @memberOf std.graphics.InputBinding
              *
              *  Add an action which takes a single value based on
              *  the event.
              */
             addToggleAction: function(name, action, onval, offval) {
                 this.actions[name] = wrapToggleAction(action, onval, offval);
             },

             /** @memberOf std.graphics.InputBinding
              *
              *  Add an action which takes a pair of floats based on the event.
              */
             addFloat2Action: function(name, action) {
                 this.actions[name] = wrapFloat2Action(action);
             },

             /** @memberOf std.graphics.InputBinding
              *  @returns true if the event was handled, false otherwise.
              *
              *  Dispatch an event, triggering an action if one is associated with the event.
              */
             dispatch: function(evt) {
                 if (!evt) return false;
                 var class_binding = this.bindings[evt.msg];
                 if (!class_binding) return false;

                 // Button or axis forms primary key
                 var key;
                 if ('button' in evt) key = evt.button;
                 if ('axis' in evt) key = evt.axis;
                 var keyed_binding = this.bindings[evt.msg][key];
                 if (!keyed_binding) return false;

                 // For modifiers, we check if we have any matches on
                 // specific modifiers. To check 'none' in the same
                 // way, we need to fill in the none field in the
                 // event modifiers. Finally, we always trigger 'any',
                 // so we also fill it in as true.
                 var mods = evt.modifier;
                 if (!mods) mods = {
                     alt: false, ctrl: false, shift: false, 'super': false
                 };
                 mods.none = (!mods.alt && !mods.ctrl && !mods.shift && !mods.super);
                 mods.any = true;

                 var handled = false;
                 for(var mod in mods) {
                     if (!mods[mod]) continue;
                     var mod_keyed_binding = keyed_binding[mod];
                     if (mod_keyed_binding && mod_keyed_binding.length > 0) {
                         for(var act_idx in mod_keyed_binding) {
                             this.actions[ mod_keyed_binding[act_idx] ](evt);
                             handled = true;
                         }
                     }
                 }
                 return handled;
             }
         }
     );

})();