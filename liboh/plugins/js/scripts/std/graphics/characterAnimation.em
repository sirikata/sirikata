// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

system.require('std/core/namespace.em');

(function() {
     var ns = Namespace('std.graphics');

     /** Performs simple character animation, extracting an animation
      *  list if possible and trying to toggle the correct animations
      *  given different states.
      */
     ns.CharacterAnimation = system.Class.extend(
         {
             // Movement states that will map to animations
             STATE_IDLE: 'idle',
             STATE_WALKING: 'walk',
             STATE_LIST: ['idle', 'walk'],

             init: function(parent) {
                 this._parent = parent;
                 this._lastState = undefined;

                 // Force one initial update to get things rolling.
                 this.update();
             },

             update: function() {
                 var new_state = this._getCurrentState();
                 if (new_state == this._lastState) return;

                 this._lastState = new_state;
                 var anims = this._getAnimations();
                 if (anims === undefined) return;
                 this._parent.startAnimation(this._parent.presence(), anims[this._lastState]);
             },

             // Returns the current state by looking at the current
             // properties of the object
             _getCurrentState: function() {
                 if (this._parent.presence().velocity.length() > .1)
                     return this.STATE_WALKING;
                 return this.STATE_IDLE;
             },

             // Get a map of animations. Instead of a raw list, this
             // provides a map from actions to animation names. This
             // takes care of figuring out the mapping if names don't
             // match up. Returns undefined if there are no
             // animations.
             _getAnimations: function() {
                 var raw_anims = this._parent.getAnimationList();
                 if (raw_anims === undefined || raw_anims == null || raw_anims.length < this.STATE_LIST.length)
                     return undefined;

                 // Try to match names
                 var raw_anims_map = {};
                 for(var i in raw_anims) {
                     var lower = raw_anims[i].toLowerCase();
                     raw_anims_map[lower] = raw_anims[i];
                 }
                 var has_all = true;
                 for(var a in this.STATE_LIST) {
                     if (!(this.STATE_LIST[a] in raw_anims_map))
                         has_all = false;
                 }
                 if (has_all)
                     return raw_anims_map;

                 // Otherwise, we're stuck hoping that the order is right
                 var anims = {};
                 for(var x in this.STATE_LIST)
                     anims[ this.STATE_LIST[x] ] = raw_anims[x];
                 return anims;
             }
         }
     );

 })();