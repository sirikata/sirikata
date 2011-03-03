/*  Sirikata
 *  graphics.em
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
if (typeof(std.graphics) === "undefined") std.graphics = {};

system.import('gui.em');

(
function() {

    var ns = std.graphics;

    /** The Graphics class wraps the underlying graphics simulation,
     *  allowing you to get access to input, control display options,
     *  and perform operations like picking in response to mouse
     *  clicks.
     */
    ns.Graphics = function(pres, name) {
        this._simulator = pres.runSimulation(name);
        this.inputHandler = new std.graphics.InputHandler(this);
    };

    ns.Graphics.prototype.invoke = function() {
        // Just forward manual invoke commands directly
        return this._simulator.invoke.apply(this._simulator, arguments);
    };

    /** Request that the renderer suspend rendering. It continues to exist, but doesn't use any CPU on rendering. */
    ns.Graphics.prototype.suspend = function() {
        this.invoke('suspend');
    };

    /** Request that the renderer resume rendering. If rendering wasn't suspended, has no effect. */
    ns.Graphics.prototype.resume = function() {
        this.invoke('resume');
    };

    /** Request that the renderer toggle suspended rendering. See suspend and resume for details. */
    ns.Graphics.prototype.toggleSuspend = function() {
        this.invoke('toggleSuspend');
    };

    /** Request that the OH shut itself down, i.e. that the entire application exit. */
    ns.Graphics.prototype.quit = function() {
        this.invoke('quit');
    };

    /** Request a screenshot be taken and stored on disk. */
    ns.Graphics.prototype.screenshot = function() {
        this.invoke('screenshot');
    };

    /** Request that the given URL be presented as a widget. */
    ns.Graphics.prototype.createGUI = function(name, url) {
        return new ns.GUI(simulator.invoke("createWindowFile", name, url));
    };

    /** Request that the given URL be presented as a widget. */
    ns.Graphics.prototype.createBrowser = function(name, url) {
        return new ns.GUI(simulator.invoke("createWindow", name, url));
    };

})();

// Import additional utilities that anybody using this class will need.
system.import('input.em');
