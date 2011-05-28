/*  Sirikata
 *  default.em
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

system.require('graphics.em');
system.require('std/movement/moveAndRotate.em');
system.require('std/script/scripter.em');
system.require('inputbinding.em');
system.require('drag/move.em');
system.require('drag/rotate.em');
system.require('drag/scale.em');
system.require('std/graphics/chat.em');

(
function() {

    var ns = std.graphics;

    /** @namespace
     *  The DefaultGraphics class just contains some sane defaults for
     *  interaction, allowing you to get a decent, baseline client
     *  that only requires built-in functionality. You still define
     *  the presence and which underlying graphics system to use, but
     *  this class takes care of defining all other UI and interaction.
     */
    std.graphics.DefaultGraphics = function(pres, name) {
        this._pres = pres;
        this._simulator = new std.graphics.Graphics(pres, name, std.core.bind(this.finishInit, this));
    };
    std.graphics.DefaultGraphics.prototype.finishInit = function(gfx) {
        // assert(gfx == this._simulator);
        this._cameraMode = 'first';

        this._selected = null;
        this._scripter = new std.script.Scripter(this);
        this._chat = new std.graphics.Chat(this._pres, this._simulator);
        this._moverot = new std.movement.MoveAndRotate(this._pres, std.core.bind(this.updateCameraOffset, this), 'rotation');

        this._draggers = {
            move: new std.graphics.MoveDragHandler(this._simulator),
            rotate: new std.graphics.RotateDragHandler(this._simulator),
            scale: new std.graphics.ScaleDragHandler(this._simulator)
        };


        this._binding = new std.graphics.InputBinding();
        this._simulator.inputHandler.onAnything = std.core.bind(this._binding.dispatch, this._binding);


        this._binding.addAction('quit', std.core.bind(this._simulator.quit, this._simulator));
        this._binding.addAction('screenshot', std.core.bind(this._simulator.screenshot, this._simulator));
        this._binding.addAction('toggleSuspend', std.core.bind(this._simulator.toggleSuspend, this._simulator));
        this._binding.addAction('scriptSelectedObject', std.core.bind(this.scriptSelectedObject, this));
        this._binding.addAction('scriptSelf', std.core.bind(this.scriptSelf, this));

        this._binding.addAction('toggleChat', std.core.bind(this.toggleChat, this));

        this._binding.addAction('toggleCameraMode', std.core.bind(this.toggleCameraMode, this));

        this._binding.addToggleAction('moveForward', std.core.bind(this.moveSelf, this, new util.Vec3(0, 0, -1)), 1, -1);
        this._binding.addToggleAction('moveBackward', std.core.bind(this.moveSelf, this, new util.Vec3(0, 0, 1)), 1, -1);
        this._binding.addToggleAction('moveLeft', std.core.bind(this.moveSelf, this, new util.Vec3(-1, 0, 0)), 1, -1);
        this._binding.addToggleAction('moveRight', std.core.bind(this.moveSelf, this, new util.Vec3(1, 0, 0)), 1, -1);
        this._binding.addToggleAction('moveUp', std.core.bind(this.moveSelf, this, new util.Vec3(0, 1, 0)), 1, -1);
        this._binding.addToggleAction('moveDown', std.core.bind(this.moveSelf, this, new util.Vec3(0, -1, 0)), 1, -1);

        this._binding.addToggleAction('rotateUp', std.core.bind(this.rotateSelf, this, new util.Vec3(1, 0, 0)), 1, -1);
        this._binding.addToggleAction('rotateDown', std.core.bind(this.rotateSelf, this, new util.Vec3(-1, 0, 0)), 1, -1);
        this._binding.addToggleAction('rotateLeft', std.core.bind(this.rotateSelf, this, new util.Vec3(0, 1, 0)), 1, -1);
        this._binding.addToggleAction('rotateRight', std.core.bind(this.rotateSelf, this, new util.Vec3(0, -1, 0)), 1, -1);

        this._binding.addFloat2Action('pickObject', std.core.bind(this.pickObject, this));

        this._binding.addAction('startMoveDrag', std.core.bind(this.startDrag, this, this._draggers.move));
        this._binding.addAction('startRotateDrag', std.core.bind(this.startDrag, this, this._draggers.rotate));
        this._binding.addAction('startScaleDrag', std.core.bind(this.startDrag, this, this._draggers.scale));
        this._binding.addAction('forwardMousePressToDragger', std.core.bind(this.forwardMousePressToDragger, this));
        this._binding.addAction('forwardMouseDragToDragger', std.core.bind(this.forwardMouseDragToDragger, this));
        this._binding.addAction('forwardMouseReleaseToDragger', std.core.bind(this.forwardMouseReleaseToDragger, this));
        this._binding.addAction('stopDrag', std.core.bind(this.stopDrag, this));

        /** Bindings are an *ordered* list of keys and actions. Keys
         *  are a combination of the type of event, the primary key
         *  for the event (key or mouse button), and modifiers.
         *  Modifiers are tricky to handle. You can specify a string
         *  indicating which to filter on. If you omit the string, it
         *  is equivalent to 'none'. The special values '*' and 'any'
         *  are equivalent and will match any modifier, i.e. the event
         *  will always be triggered. There isn't support
         *  for matching combinations of modifiers.
         */
        var bindings = [
            { key: ['button-pressed', 'escape'], action: 'quit' },
            { key: ['button-pressed', 'i'], action: 'screenshot' },
            { key: ['button-pressed', 'm'], action: 'toggleSuspend' },
            { key: ['button-pressed', 's', 'alt' ], action: 'scriptSelectedObject' },
            { key: ['button-pressed', 's', 'ctrl' ], action: 'scriptSelf' },

            { key: ['button-pressed', 'c', 'ctrl' ], action: 'toggleChat' },

            { key: ['mouse-click', 3], action: 'pickObject' },
            { key: ['mouse-click', 3], action: 'scriptSelectedObject' },

            { key: ['button-pressed', 'c' ], action: 'toggleCameraMode' },

            { key: ['button', 'w' ], action: 'moveForward' },
            { key: ['button', 'up' ], action: 'moveForward' },
            { key: ['button', 's' ], action: 'moveBackward' },
            { key: ['button', 'down' ], action: 'moveBackward' },
            { key: ['button', 'a' ], action: 'moveLeft' },
            { key: ['button', 'd' ], action: 'moveRight' },
            { key: ['button', 'q' ], action: 'moveUp' },
            { key: ['button', 'z' ], action: 'moveDown' },

            { key: ['button', 'up', 'shift' ], action: 'rotateUp' },
            { key: ['button', 'down', 'shift' ], action: 'rotateDown' },
            { key: ['button', 'left' ], action: 'rotateLeft' },
            { key: ['button', 'right' ], action: 'rotateRight' },

            { key: ['mouse-press', 1 ], action: 'pickObject' },

            // Note that the ordering of registration here is critical.
            { key: ['mouse-press', 1, 'none' ], action: 'startMoveDrag' },
            { key: ['mouse-press', 1, 'ctrl' ], action: 'startRotateDrag' },
            { key: ['mouse-press', 1, 'alt' ], action: 'startScaleDrag' },
            { key: ['mouse-press', 1, '*'], action: 'forwardMousePressToDragger' },
            { key: ['mouse-drag', 1, '*'], action: 'forwardMouseDragToDragger' },
            { key: ['mouse-release', 1, '*'], action: 'forwardMouseReleaseToDragger' },
            { key: ['mouse-release', 1, '*'], action: 'stopDrag' }
        ];

        this._binding.addBindings(bindings);
    };

    /** @function */
    std.graphics.DefaultGraphics.prototype.simulator = function() {
        return this._simulator;
    };

    /** @function */
    std.graphics.DefaultGraphics.prototype.invoke = function() {
        // Just forward manual invoke commands directly
        return this._simulator.invoke.apply(this._simulator, arguments);
    };

    /** Request that the given URL be added as a module in the UI. */
    std.graphics.DefaultGraphics.prototype.addGUIModule = function(name, url, cb) {
        return this._simulator.addGUIModule(name, url, cb);
    };


    //by default how to scale translational velocity from keypresses.  (movement
    //is agonizingly slow if just set this to 1.  I really recommend 5.)
    /** @public */
    std.graphics.DefaultGraphics.prototype.defaultVelocityScaling = 5;
    //by default how to scale rotational velocity from keypresses
    /** @public */
    std.graphics.DefaultGraphics.prototype.defaultRotationalVelocityScaling = .5;


    /** @function */
    std.graphics.DefaultGraphics.prototype.toggleChat = function() {
        this._chat.toggle();
    };

    /** @function */
    std.graphics.DefaultGraphics.prototype.scriptSelf = function() {
        this._scripter.script(system.self);
    };

    /** @function */
    std.graphics.DefaultGraphics.prototype.scriptSelectedObject = function() {
        this._scripter.script(this._selected);
    };

    /** @function */
    std.graphics.DefaultGraphics.prototype.toggleCameraMode = function(evt) {
        this._cameraMode = this._cameraMode == 'first' ? 'third' : 'first';
        this._simulator.setCameraMode(this._cameraMode);
        this.updateCameraOffset();
    };

    /** @function */
    std.graphics.DefaultGraphics.prototype.updateCameraOffset = function(evt) {
        if (this._cameraMode == 'third') {
            var orient = this._pres.getOrientation();
            this._simulator.setCameraOffset(orient.mul(<0, 1.5, 4>));
        }
    };

    /** @function */
    std.graphics.DefaultGraphics.prototype.moveSelf = function(dir, val) {
        this._moverot.move(dir, this.defaultVelocityScaling * val);
    };

    /** @function */
    std.graphics.DefaultGraphics.prototype.rotateSelf = function(about, val) {
        this._moverot.rotate(about, this.defaultRotationalVelocityScaling * val);
    };

    /** @function */
    std.graphics.DefaultGraphics.prototype.pickObject = function(x, y) {
        if (this._selected) {
            this._simulator.bbox(this._selected, false);
            this._selected = null;
        }

        var clicked = this._simulator.pick(x, y);
        if (clicked) {
            this._selected = clicked;
            this._simulator.bbox(this._selected, true);
        }
    };

    /** @function */
    std.graphics.DefaultGraphics.prototype.startDrag = function(dragger, evt) {
        if (this._selected)
            this._dragger = dragger;
        if (this._dragger)
            this._dragger.selected(this._selected, evt);
    };

    /** @function */
    std.graphics.DefaultGraphics.prototype.forwardMousePressToDragger = function(evt) {
        if (this._dragger) this._dragger.onMousePress(evt);
    };

    /** @function */
    std.graphics.DefaultGraphics.prototype.forwardMouseDragToDragger = function(evt) {
        if (this._dragger) this._dragger.onMouseDrag(evt);
    };

    /** @function */
    std.graphics.DefaultGraphics.prototype.forwardMouseReleaseToDragger = function(evt) {
        if (this._dragger) this._dragger.onMouseRelease(evt);
    };

    /** @function */
    std.graphics.DefaultGraphics.prototype.stopDrag = function() {
        delete this._dragger;
    };

})();
