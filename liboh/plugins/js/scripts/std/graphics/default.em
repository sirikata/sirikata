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
system.require('std/movement/movement.em');
system.require('std/script/scripter.em');
system.require('drag/move.em');
system.require('drag/rotate.em');
system.require('drag/scale.em');

(
function() {

    var ns = std.graphics;

    /** The DefaultGraphics class just contains some sane defaults for
     *  interaction, allowing you to get a decent, baseline client
     *  that only requires built-in functionality. You still define
     *  the presence and which underlying graphics system to use, but
     *  this class takes care of defining all other UI and interaction.
     */
    ns.DefaultGraphics = function(pres, name) {
        this._pres = pres;
        this._simulator = new std.graphics.Graphics(pres, name);
        this._simulator.inputHandler.onMouseClick = std.core.bind(this.onMouseClick, this);
        this._simulator.inputHandler.onButtonPressed = std.core.bind(this.onButtonPressed, this);
        this._simulator.inputHandler.onButtonReleased = std.core.bind(this.onButtonReleased, this);
        this._simulator.inputHandler.onButtonRepeated = std.core.bind(this.onButtonRepeated, this);
        this._simulator.inputHandler.onMouseDrag = std.core.bind(this.onMouseDrag, this);
        this._simulator.inputHandler.onMouseClick = std.core.bind(this.onMouseClick, this);
        this._simulator.inputHandler.onMousePress = std.core.bind(this.onMousePress, this);
        this._simulator.inputHandler.onMouseRelease = std.core.bind(this.onMouseRelease, this);

        this._selected = null;
        this._scripter = new std.script.Scripter(this);

        this._draggers = {
            move: new std.graphics.MoveDragHandler(this._simulator),
            rotate: new std.graphics.RotateDragHandler(this._simulator),
            scale: new std.graphics.ScaleDragHandler(this._simulator)
        };
    };

    ns.DefaultGraphics.prototype.invoke = function() {
        // Just forward manual invoke commands directly
        return this._simulator.invoke.apply(this._simulator, arguments);
    };


    //by default how to scale translational velocity from keypresses.  (movement
    //is agonizingly slow if just set this to 1.  I really recommend 5.)
    ns.DefaultGraphics.prototype.defaultVelocityScaling = 5;
    //by default how to scale rotational velocity from keypresses
    ns.DefaultGraphics.prototype.defaultRotationVelocityScaling = 1;

    ns.DefaultGraphics.prototype.onButtonPressed = function(evt) {
        if (evt.button == 'escape') this._simulator.quit();
        if (evt.button == 'i') this._simulator.screenshot();
        if (evt.button == 'm') this._simulator.toggleSuspend();

        if (evt.button == 'up' && !evt.modifier.shift) std.movement.move(this._pres, new util.Vec3(0, 0, -1), this.defaultVelocityScaling);
        if (evt.button == 'down' && !evt.modifier.shift) std.movement.move(this._pres, new util.Vec3(0, 0, 1), this.defaultVelocityScaling);

        if (evt.button == 'up' && evt.modifier.shift) std.movement.rotate(this._pres, new util.Vec3(1, 0, 0), this.defaultVelocityScaling);
        if (evt.button == 'down' && evt.modifier.shift) std.movement.rotate(this._pres, new util.Vec3(-1, 0, 0), this.defaultVelocityScaling);
        if (evt.button == 'left') std.movement.rotate(this._pres, new util.Vec3(0, 1, 0), this.defaultRotationVelocityScaling);
        if (evt.button == 'right') std.movement.rotate(this._pres, new util.Vec3(0, -1, 0), this.defaultRotationVelocityScaling);

        if (evt.button == 'w') std.movement.move(this._pres, new util.Vec3(0, 0, -1), 1);
        if (evt.button == 's' && !evt.modifier.alt && !evt.modifier.ctrl) std.movement.move(this._pres, new util.Vec3(0, 0, 1), 1);
        if (evt.button == 'a') std.movement.move(this._pres, new util.Vec3(-1, 0, 0), 1);
        if (evt.button == 'd') std.movement.move(this._pres, new util.Vec3(1, 0, 0), 1);
        if (evt.button == 'q') std.movement.move(this._pres, new util.Vec3(0, 1, 0), 1);
        if (evt.button == 'z') std.movement.move(this._pres, new util.Vec3(0, -1, 0), 1);

        if (evt.button == 's' && evt.modifier.alt)
            this._scripter.script(this._selected);

        if (evt.button == 's' && evt.modifier.ctrl)
            this._scripter.script(system.Self);
    };

    ns.DefaultGraphics.prototype.onButtonRepeated = function(evt) {
    };

    ns.DefaultGraphics.prototype.onButtonReleased = function(evt) {
        std.movement.stopMove(this._pres);
        std.movement.stopRotate(this._pres);
    };

    ns.DefaultGraphics.prototype.onMousePress = function(evt) {
        if (this._selected) {
            this._simulator.bbox(this._selected, false);
            this._selected = null;
        }

        var clicked = this._simulator.pick(evt.x, evt.y);
        if (clicked) {
            this._selected = clicked;
            this._simulator.bbox(this._selected, true);
        }

        if (evt.button == 1) {
            if (evt.modifier.ctrl)
                this._dragger = this._draggers.rotate;
            else if (evt.modifier.alt)
                this._dragger = this._draggers.scale;
            else
                this._dragger = this._draggers.move;
        }
        if (this._dragger) {
            this._dragger.selected(this._selected);
            this._dragger.onMousePress(evt);
        }
    };

    ns.DefaultGraphics.prototype.onMouseDrag = function(evt) {
        if (this._dragger) this._dragger.onMouseDrag(evt);
    };

    ns.DefaultGraphics.prototype.onMouseRelease = function(evt) {
        if (this._dragger) this._dragger.onMouseRelease(evt);
        delete this._dragger;
    };

    ns.DefaultGraphics.prototype.onMouseClick = function(evt) {
    };

})();
