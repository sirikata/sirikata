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

system.require('std/core/namespace.em');
system.require('std/graphics/graphics.em');
system.require('std/graphics/undo.em');
system.require('std/movement/pursue.em');
system.require('std/graphics/inputbinding.em');
system.require('std/graphics/defaultcamera.em');
system.require('std/graphics/drag/move.em');
system.require('std/graphics/drag/rotate.em');
system.require('std/graphics/drag/scale.em');
system.require('std/graphics/chat.em');
system.require('std/graphics/physics.em');
system.require('std/graphics/propertybox.em');
system.require('std/graphics/presenceList.em');
system.require('std/graphics/setMesh.em');
system.require('std/graphics/axes.em');
system.require('std/graphics/flatlandViewer.em');
system.require('std/graphics/characterAnimation.em');

system.require('std/audio/audio.em');
system.require('std/env/env.em');


(
function() {

    var ns = Namespace('std.client');

    /** @namespace
     *  The Default class just contains some sane defaults for
     *  interaction, allowing you to get a decent, baseline client
     *  that only requires built-in functionality. You still define
     *  the presence and which underlying graphics system to use, but
     *  this class takes care of defining all other UI and interaction.
     */
    std.client.Default = function(pres, cb) {
        this._pres = pres;

        // We'd like to rename this this._graphics, but many other things depend on it's location still...
        this._simulator = new std.graphics.Graphics(pres, 'ogregraphics', std.core.bind(this.finishedGraphicsInit, this, cb), std.core.bind(this.finishedGraphicsUIReset, this));

        this._env = new std.env.Environment(pres);
        this._env.listen(std.core.bind(this._environmentChanged, this));

        this._audio = new std.audio.Audio(pres, 'sdlaudio');
    };

    
    std.client.Default.prototype.finishedGraphicsInit = function(cb, gfx, alreadyInitialized) {
        this._camera = new std.graphics.DefaultCamera(this._simulator, system.self);
        
        this._selected = null;
        this._loadingUIs = 0;

        this._alreadyInitialized = alreadyInitialized;
        if (! this._alreadyInitialized)
        {
            var ui_finish_cb = std.core.bind(this.finishedUIInit, this, cb);
//            this._loadingUIs++; this._scripter = new std.script.Scripter(this, ui_finish_cb);
            //using new scripting gui interface
            // this._loadingUIs++; this._scripter = new std.ScriptingGui.Controller(this);

            
            //this._loadingUIs++; this._chat = new std.graphics.Chat(this._pres, this._simulator, ui_finish_cb);
            this._loadingUIs++; this._physics = new std.graphics.PhysicsProperties(this._simulator, ui_finish_cb);
            //this._loadingUIs++; this._propertybox = new std.propertybox.PropertyBox(this, ui_finish_cb);
            //this._loadingUIs++; this._presenceList = new std.graphics.PresenceList(this._pres, this._simulator, this._scripter, ui_finish_cb);
            //this._loadingUIs++; this._setMesh = new std.graphics.SetMesh(this._simulator, ui_finish_cb);
            //this._loadingUIs++; this._flatland = new std.fl.FL(this, ui_finish_cb);                
        }
        else
            this.finishedUIInit(cb);

    };

    std.client.Default.prototype.finishedGraphicsUIReset = function(gfx) {
        this._camera.reinit();

        var ui_finish_cb = std.core.bind(this.finishedUIInit, this);

        //this._loadingUIs++; this._scripter.onReset(ui_finish_cb);
        //this._loadingUIs++; this._chat.onReset(ui_finish_cb);
        this._loadingUIs++; this._physics.onReset(ui_finish_cb);
        //this._loadingUIs++; this._propertybox.onReset(ui_finish_cb);
        //this._loadingUIs++; this._presenceList.onReset(ui_finish_cb);
        //this._loadingUIs++; this._setMesh.onReset(ui_finish_cb);
        //this._loadingUIs++; this._flatland.onReset(ui_finish_cb);
    };


    /**
     this._alreadyInitialized is true or false.  If true, means that we
     already had a graphics viewer completely initialized before going
     through this initialization process.  If false, means that we
     are doing initialization for the first time.
     */
    std.client.Default.prototype.finishedUIInit = function(cb) {
        this._loadingUIs--;
        if (this._loadingUIs > 0) return;


        if (! this._alreadyInitialized)
            this._simulator.hideLoadScreen();

        this._moverot = new std.movement.MoveAndRotate(this._pres, std.core.bind(this.updateCameraAndAnimation, this), 'rotation');
        this._characterAnimation = new std.graphics.CharacterAnimation(this);

        this._draggers = {
            move: new std.graphics.MoveDragHandler(this._simulator),
            rotate: new std.graphics.RotateDragHandler(this._simulator),
            scale: new std.graphics.ScaleDragHandler(this._simulator)
        };

        this._binding = new std.graphics.InputBinding();
        this._simulator.inputHandler.onAnything = std.core.bind(this._binding.dispatch, this._binding);

        this._binding.addFloat2Action('pickObject', std.core.bind(this.pickObject, this));

        if (! this._alreadyInitialized)
        {
            this._binding.addAction('quit', std.core.bind(this._simulator.quit, this._simulator));
            this._binding.addAction('screenshot', std.core.bind(this._simulator.screenshot, this._simulator));
            this._binding.addAction('toggleSuspend', std.core.bind(this._simulator.toggleSuspend, this._simulator));
            this._binding.addAction('scriptSelectedObject', std.core.bind(this.scriptSelectedObject, this));
            this._binding.addAction('scriptSelf', std.core.bind(this.scriptSelf, this));

            
            this._binding.addAction('togglePropertyBox', std.core.bind(this.togglePropertyBox, this));
            this._binding.addAction('toggleChat', std.core.bind(this.toggleChat, this));
            this._binding.addAction('togglePhysicsProperties', std.core.bind(this._physics.toggle, this._physics));
            //this._binding.addAction('togglePresenceList', std.core.bind(this._presenceList.toggle, this._presenceList));
            //this._binding.addAction('toggleSetMesh', std.core.bind(this._setMesh.toggle, this._setMesh));
            //this._binding.addFloat2Action('showFlatland', std.core.bind(this.showFlatland, this));
            //this._binding.addAction('hideFlatland', std.core.bind(this.hideFlatland, this));

            this._binding.addAction('toggleCameraMode', std.core.bind(this.toggleCameraMode, this));

            this._binding.addAction('actOnObject', std.core.bind(this.actOnObject, this));
            this._binding.addAction('teleportToObj', std.core.bind(this.teleportToObj, this));

            this._binding.addToggleAction('moveForward', std.core.bind(this.moveSelf, this, new util.Vec3(0, 0, -1)), 1, -1);
            this._binding.addToggleAction('moveBackward', std.core.bind(this.moveSelf, this, new util.Vec3(0, 0, 1)), 1, -1);
            this._binding.addToggleAction('moveLeft', std.core.bind(this.moveSelf, this, new util.Vec3(-1, 0, 0)), 1, -1);
            this._binding.addToggleAction('moveRight', std.core.bind(this.moveSelf, this, new util.Vec3(1, 0, 0)), 1, -1);
            this._binding.addToggleAction('moveUp', std.core.bind(this.moveSelf, this, new util.Vec3(0, 1, 0)), 1, -1);
            this._binding.addToggleAction('moveDown', std.core.bind(this.moveSelf, this, new util.Vec3(0, -1, 0)), 1, -1);

            this._binding.addToggleAction('rotateUp', std.core.bind(this.rotateSelf, this, new util.Vec3(1, 0, 0), "_rotate_up_handle_"), true, false);
            this._binding.addToggleAction('rotateDown', std.core.bind(this.rotateSelf, this, new util.Vec3(-1, 0, 0), "_rotate_down_handle_"), true, false);
            this._binding.addToggleAction('rotateLeft', std.core.bind(this.rotateSelf, this, new util.Vec3(0, 1, 0), "_rotate_left_handle_"), true, false);
            this._binding.addToggleAction('rotateRight', std.core.bind(this.rotateSelf, this, new util.Vec3(0, -1, 0), "_rotate_right_handle_"), true, false);

            this._binding.addFloat2Action('turnOffAxis', std.core.bind(this.turnOffAxis, this));
        
            this._binding.addAction('axesSnapLocal', std.core.bind(this.setAxesInheritOrient, this, true));
            this._binding.addAction('axesSnapGlobal', std.core.bind(this.setAxesInheritOrient, this, false));
        
            this._binding.addAction('updatePhysicsProperties', std.core.bind(this.updatePhysicsProperties, this));

            this._binding.addAction('startMoveDrag', std.core.bind(this.startDrag, this, this._draggers.move));
            this._binding.addAction('startRotateDrag', std.core.bind(this.startDrag, this, this._draggers.rotate));
            this._binding.addAction('startScaleDrag', std.core.bind(this.startDrag, this, this._draggers.scale));
            this._binding.addAction('forwardMousePressToDragger', std.core.bind(this.forwardMousePressToDragger, this));
            this._binding.addAction('forwardMouseDragToDragger', std.core.bind(this.forwardMouseDragToDragger, this));
            this._binding.addAction('updatePropertyBox', std.core.bind(this.updatePropertyBox, this));
            this._binding.addAction('forwardMouseReleaseToDragger', std.core.bind(this.forwardMouseReleaseToDragger, this));
            this._binding.addAction('stopDrag', std.core.bind(this.stopDrag, this));

            this._binding.addAction('startFreeRotate', std.core.bind(this.startFreeRotate, this));
            this._binding.addAction('freeRotateDrag', std.core.bind(this.freeRotateDrag, this));
            this._binding.addAction('freeRotateRelease', std.core.bind(this.freeRotateRelease, this));
            this._binding.addAction('undo', std.core.bind(this.undo, this));
            this._binding.addAction('redo', std.core.bind(this.redo, this));
        }

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
        var bindings = [{ key: ['mouse-press', 1 ], action: 'pickObject' },
                        { key: ['mouse-click', 2], action: 'pickObject' }];

        if (!this._alreadyInitialized)
        {
            bindings = bindings.concat([{ key: ['button-pressed', 'escape'], action: 'quit' },
                                        { key: ['button-pressed', 'i'], action: 'screenshot' },
                                        { key: ['button-pressed', 'm'], action: 'toggleSuspend' },
                                        { key: ['button-pressed', 's', 'alt' ], action: 'scriptSelectedObject' },
                                        { key: ['button-pressed', 's', 'ctrl' ], action: 'scriptSelf' },
                                        { key: ['button-pressed', 'c', 'ctrl' ], action: 'toggleChat' },
                                        { key: ['button-pressed', 'p', 'ctrl' ], action: 'togglePhysicsProperties' },
                                        { key: ['button-pressed', 'p', 'alt' ], action: 'togglePropertyBox' },
                                        { key: ['button-pressed', 'l', 'ctrl' ], action: 'togglePresenceList' },
                                        { key: ['button-pressed', 'j', 'ctrl' ], action: 'toggleSetMesh' },
            
                                        { key: ['button-pressed', 'g', 'alt' ], action: 'axesSnapLocal' },
                                        { key: ['button-pressed', 'g', 'ctrl' ], action: 'axesSnapGlobal' },
                                        
                                        { key: ['button-pressed', 'z', 'ctrl' ], action: 'undo' },
                                        { key: ['button-pressed', 'y', 'ctrl' ], action: 'redo' },
                                        
                                        { key: ['mouse-click', 1, 'shift'], action: 'turnOffAxis' },
                                        { key: ['mouse-click', 2], action: 'scriptSelectedObject' },
                                        { key: ['button-pressed', 'return'], action: 'actOnObject' },
                                        { key: ['button-pressed', 't', 'ctrl' ], action: 'teleportToObj' },

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

                                        { key: ['mouse-press', 1 ], action: 'updatePhysicsProperties' },

                                        // Note that the ordering of registration here is critical.
                                        { key: ['mouse-press', 1, 'none' ], action: 'startMoveDrag' },
                                        { key: ['mouse-press', 1, 'ctrl' ], action: 'startRotateDrag' },
                                        { key: ['mouse-press', 1, 'alt' ], action: 'startScaleDrag' },
                                        { key: ['mouse-press', 1, '*'], action: 'forwardMousePressToDragger' },
                                        { key: ['mouse-press', 1, '*'], action: 'updatePropertyBox' },
                                        { key: ['mouse-drag', 1, '*'], action: 'forwardMouseDragToDragger' },
                                        { key: ['mouse-release', 1, '*'], action: 'forwardMouseReleaseToDragger' },
                                        { key: ['mouse-release', 1, '*'], action: 'stopDrag' },
                                        { key: ['mouse-press', 3, 'none'], action: 'showFlatland' },
                                        { key: ['mouse-press', 3, 'none' ], action: 'startFreeRotate' },
                                        { key: ['mouse-drag', 3, 'none'], action: 'freeRotateDrag' },
                                        { key: ['mouse-release', 3, 'none'], action: 'freeRotateRelease' }
                                       ]);
        }

        this._binding.addBindings(bindings);
        std.graphics.axes.init(this._simulator);
        this._axesInheritOrient = true;

        if (cb && typeof(cb) === "function")
            cb(this);
    };

    std.client.Default.prototype.presence = function() {
        return this._pres;
    };

    /** @function */
    std.client.Default.prototype.simulator = function() {
        return this._simulator;
    };

    /** @function */
    std.client.Default.prototype.invoke = function() {
        // Just forward manual invoke commands directly
        return this._simulator.invoke.apply(this._simulator, arguments);
    };

    /** Request that the given URL be added as a module in the UI. */
    std.client.Default.prototype.addGUIModule = function(name, url, cb) {
        return this._simulator.addGUIModule(name, url, cb);
    };

    /** Request that the given script text be added as a module in the UI. */
    std.client.Default.prototype.addGUITextModule = function(name, js_text, cb) {
        return this._simulator.addGUITextModule(name, js_text, cb);
    };

    /** Get the GUI for a previously loaded GUI module. */
    std.client.Default.prototype.getGUIModule = function(name) {
        return this._simulator.getGUIModule(name);
    };

    /** Request that the given URL be presented as a widget. */
    std.client.Default.prototype.createBrowser = function(name, url, cb) {
        return this._simulator.createBrowser(name, url, cb);
    };


    /** Get a list of animations associated with this entity. */
    std.client.Default.prototype.getAnimationList = function(vis) {
        if (!vis)
          vis = system.self;

        return this._simulator.getAnimationList(vis);
    };

    /** Start the animation on this entity given by the specified animation name. */
    std.client.Default.prototype.startAnimation = function(vis, anim) {
        return this._simulator.startAnimation(vis, anim);
    };

    /** Stop the animation on this entity. */
    std.client.Default.prototype.stopAnimation = function(vis) {
        return this._simulator.stopAnimation(vis);
    };



    //by default how to scale translational velocity from keypresses.  (movement
    //is agonizingly slow if just set this to 1.  I really recommend 5.)
    /** @public */
    std.client.Default.prototype.defaultVelocityScaling = 5;
    //by default how to scale rotational velocity from keypresses
    /** @public */
    std.client.Default.prototype.defaultRotationalVelocityScaling = .5;


    /** @function */
    std.client.Default.prototype.toggleChat = function() {
        this._chat.toggle();
    };

    /** @function */
    std.client.Default.prototype.scriptSelf = function() {
        if (typeof(this._scripter) == 'undefined')
        {
            system.require('std/scriptingGui/scriptingGui.em');
            this._scripter = new std.ScriptingGui.Controller(this);
        }
        
        this._scripter.script(system.self);
    };

    /** @function */
    std.client.Default.prototype.togglePropertyBox = function() {
        this._propertybox.TogglePropertyBox();
    };

    std.client.Default.prototype.setAxesInheritOrient = function(inheritOrient) {
        if (this._axesInheritOrient === inheritOrient) {
            return;
        }
        
        this._axesInheritOrient = inheritOrient;
        
        if (this._selected) {
            std.graphics.axes.setInheritOrientAll(this._selected, inheritOrient);
        }
    };

    /** @function */
    std.client.Default.prototype.scriptSelectedObject = function() {
        if (this._selected == null)
            return;
        //this._presenceList.addObject(this._selected.toString(), 'Scripted');
        if (typeof(this._scripter) == 'undefined')
        {
            //gross way of doing this.  allows us to use simpleInput.em
            system.require('std/scriptingGui/scriptingGui.em');
            this._scripter = new std.ScriptingGui.Controller(this);
        }

        this._scripter.script(this._selected);
    };

    /** @function */
    std.client.Default.prototype.toggleCameraMode = function(evt) {
        var newmode = this._camera.mode() == 'first' ? 'third' : 'first';
        this._camera.setMode(newmode);
        this.updateCameraOffset();
    };

    /** @function */
    std.client.Default.prototype.updateCameraOffset = function() {
        if (this._camera.mode() == 'third') {
            var orient = this._pres.getOrientation();
            this._camera.setOffset(orient.mul(<0, 1.5, 6>));
        }
    };

    std.client.Default.prototype.updateAnimation = function() {
        this._characterAnimation.update();
    };

    std.client.Default.prototype.updateCameraAndAnimation = function() {
        this.updateCameraOffset();
        this.updateAnimation();
    };

    /** @function */
    std.client.Default.prototype.moveSelf = function(dir, val) {
        this._moverot.move(dir, this.defaultVelocityScaling * val);
    };

    /** @function */
    std.client.Default.prototype.rotateSelf = function(about, name, val) {
        if (val) {
            this[name] = this._moverot.rotate(about, this.defaultRotationalVelocityScaling);
        }
        else {
            this._moverot.cancelRotate(this[name]);
            delete this[name];
        }
    };

    std.client.Default.prototype.turnOffAxis = function(x, y) {
        if (!this._selected) {
            return;
        }
        var axis = std.graphics.axes.pick(this._selected, x, y);
        if (axis < 0) {
            return;
        } else {
            std.graphics.axes.setVisible(this._selected, axis, false);
        }
    };

    /** @function */
    std.client.Default.prototype.pickObject = function(x, y)
    {
        var ignore_self = this._camera.mode() == 'first';
        var clicked = this._simulator.pick(x, y, ignore_self);
        if (clicked) {
            if (this._selected) {
                if (this._selected.toString() != clicked.toString()) {
                    if (!this._alreadyInitialized)
                    {
                        this._simulator.bbox(this._selected, false);
                        std.graphics.axes.setVisibleAll(this._selected, false);                        
                    }
                } else {
                    return;
                }
            }

            this._selected = clicked;
            if (!this._alreadyInitialized)
            {
                this._simulator.bbox(this._selected, true);
                std.graphics.axes.setInheritOrientAll(this._selected, this._axesInheritOrient);
                std.graphics.axes.setVisibleAll(this._selected, true);
            }
        } else if (this._selected) {
            if (!this._alreadyInitialized)
            {
                this._simulator.bbox(this._selected, false);
                std.graphics.axes.setVisibleAll(this._selected, false);
            }
            this._selected = null;
        }

    };

    /** @function */
    std.client.Default.prototype.actOnObject = function(evt) {
        if (this._selected)
            { 'action' : 'touch' } >> this._selected >> [];
    }
    
    /** @function */
    std.client.Default.prototype.teleportToObj = function(evt) {
        if (this._selected) {
            var dir = this._selected.getPosition() - this._pres.getPosition();
            dir = dir.normal();
            var self = this;
            this._selected.loadMesh(function() {
                var hit = std.raytrace.raytrace(null, self._pres.getPosition(), dir, self._selected, null);
                if (hit) {
                    self._pres.setPosition(hit.sub(dir.scale(3)));
                } else {
                    self._pres.setPosition(this._selected.getPosition());
                }
                self._pres.setOrientation(util.Quaternion.fromLookAt(dir));
            });
        }
    }

    /** @function */
    std.client.Default.prototype.updatePhysicsProperties = function() {
        // Update even if not selected so display can be disabled
        this._physics.update(this._selected);
    };

    /** @function */
    std.client.Default.prototype.startDrag = function(dragger, evt) {
        if (this._selected)
            this._dragger = dragger;
        if (this._dragger)
            this._dragger.selected(this._selected, this._simulator.pickedPosition(), evt);
    };

    /** @function */
    std.client.Default.prototype.forwardMousePressToDragger = function(evt) {
        if (this._dragger) this._dragger.onMousePress(evt);
    };

    /** @function */
    std.client.Default.prototype.forwardMouseDragToDragger = function(evt) {
        if (this._dragger) this._dragger.onMouseDrag(evt);
        //this._propertybox.HandleUpdateProperties(this._selected);
    };


    
    /** @function */
    std.client.Default.prototype.updatePropertyBox = function(evt) {
        //this._propertybox.HandleUpdateProperties(this._selected);
    };

    /** @function */
    std.client.Default.prototype.forwardMouseReleaseToDragger = function(evt) {
        if (this._dragger) this._dragger.onMouseRelease(evt);
    };

    /** @function */
    std.client.Default.prototype.stopDrag = function() {
        delete this._dragger;
    };

    /** @function */
    std.client.Default.prototype.startFreeRotate = function(evt) {
        this.startX = evt.x;
        this.startY = evt.y;
        this.startOrientation = this._pres.getOrientation();
    };

    /** @function */
    std.client.Default.prototype.freeRotateDrag = function(evt) {
        if (this.startX == null || this.startY == null)
            return;

        var SCALE = 1;
        var dx = evt.x - this.startX;
        var dy = evt.y - this.startY;
        var dXAngle = dx * SCALE;
        var dYAngle = dy * SCALE;
        var newQuat = this.startOrientation
            .mul(new util.Quaternion(<0, 1, 0>, -dXAngle))
            .mul(new util.Quaternion(<1, 0, 0>, dYAngle));
        this._pres.setOrientation(newQuat);

        // Reorient self so up is up.
        this._pres.setOrientation(util.Quaternion.fromLookAt(
            this._pres.getOrientation().zAxis().scale(-1), <0, 1, 0>));
        this.updateCameraOffset();
        this._moverot.reeval();
    };

    /** @function */
    std.client.Default.prototype.freeRotateRelease = function(evt) {
        this.startX = null;
        this.startY = null;
        this.startOrientation = null;
    };

    std.client.Default.prototype.undo = function(evt) {
        this._simulator.undo();
    };

    std.client.Default.prototype.redo = function(evt) {
        this._simulator.redo();
    };


    std.client.Default.prototype.orientDefault = function(evt) {
        var vis = this._selected;
        if(!vis)
            return;
        var orient = vis.orientation;
        var movable = new std.movement.MovableRemote(vis);
        movable.setOrientation(new util.Quaternion());
        this._simulator.addUndoAction({}, {
            undo: function(action) {
                movable.setOrientation(orient);
            },
            redo: function(action) {
                movable.setOrientation(new util.Quaternion());
            }
        });
    };

    std.client.Default.prototype.setMaxObjects = function(maxobjs) {
        this._simulator.setMaxObjects(maxobjs);
    };


    std.client.Default.prototype.showFlatland = function (x, y) {
        var ignore_self = this._camera.mode() == 'first';
        var clicked = this._simulator.pick(x, y, ignore_self);
        this._flatland.show(clicked, x, y);
    };

    std.client.Default.prototype.hideFlatland = function () {
        this._flatland.hide();
    };



    std.client.Default.prototype._environmentChanged = function () {
        // Check all paths we know about to see if they've changed

        // Audio
        var audio_ambient = this._env.get('audio.ambient');
        if (audio_ambient && audio_ambient != this._env.audio_ambient) {
            this._env.audio_ambient = audio_ambient;
            if (this._env.audio_ambient_clip)
                this._env.audio_ambient_clip.stop();
            // Default to volume 1, looping
            this._env.audio_ambient_clip = this._audio.play(audio_ambient, 1, true);
        }
        var audio_ambient_volume = this._env.get('audio.ambient_volume');
        if (audio_ambient_volume !== undefined && this._env.audio_ambient_clip) {
            this._env.audio_ambient_clip.volume(parseFloat(audio_ambient_volume));
        }
    };

})();
