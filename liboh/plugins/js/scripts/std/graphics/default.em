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

system.import('graphics.em');

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
        this._simulator = new std.graphics.Graphics(pres, name);
        this._simulator.inputHandler.onMouseClick = std.core.bind(this.onMouseClick, this);
        this._simulator.inputHandler.onButtonPressed = std.core.bind(this.onButtonPressed, this);
    };

    ns.DefaultGraphics.prototype.invoke = function() {
        // Just forward manual invoke commands directly
        return this._simulator.invoke.apply(this._simulator, arguments);
    };

    ns.DefaultGraphics.prototype.onMouseClick = function(evt) {
    };

    ns.DefaultGraphics.prototype.onButtonPressed = function(evt) {
        system.print(evt.button + "\n");
        if (evt.button == 'escape') this._simulator.quit();
        if (evt.button == 'i') this._simulator.screenshot();
        if (evt.button == 'm') this._simulator.toggleSuspend();
    };

})();
