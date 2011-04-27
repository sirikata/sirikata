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

system.require('gui.em');

(
function() {

    var ns = std.graphics;

    /** @namespace
     * The Graphics class wraps the underlying graphics simulation,
     *  allowing you to get access to input, control display options,
     *  and perform operations like picking in response to mouse
     *  clicks.
     */
    std.graphics.Graphics = function(pres, name) {
        this.presence = pres;
        this._simulator = pres.runSimulation(name);
        this.inputHandler = new std.graphics.InputHandler(this);
    };

    std.graphics.Graphics.prototype.invoke = function() {
        // Just forward manual invoke commands directly
        return this._simulator.invoke.apply(this._simulator, arguments);
    };

    /** Request that the renderer suspend rendering. It continues to exist, but doesn't use any CPU on rendering. */
    std.graphics.Graphics.prototype.suspend = function() {
        this.invoke('suspend');
    };

    /** Request that the renderer resume rendering. If rendering wasn't suspended, has no effect. */
    std.graphics.Graphics.prototype.resume = function() {
        this.invoke('resume');
    };

    /** Request that the renderer toggle suspended rendering. See suspend and resume for details. */
    std.graphics.Graphics.prototype.toggleSuspend = function() {
        this.invoke('toggleSuspend');
    };

    /** Request that the OH shut itself down, i.e. that the entire application exit. */
    std.graphics.Graphics.prototype.quit = function() {
        this.invoke('quit');
    };

    /** Request a screenshot be taken and stored on disk. */
    std.graphics.Graphics.prototype.screenshot = function() {
        this.invoke('screenshot');
    };

    /** Request a screenshot be taken and stored on disk. */
    std.graphics.Graphics.prototype.pick = function(x, y) {
        return this.invoke('pick', x, y);
    };

    /** Request the bounding box for the object be enabled or disabled. */
    std.graphics.Graphics.prototype.bbox = function(obj, on) {
        return this.invoke('bbox', obj, on);
    };

    /** Request that the given URL be presented as a widget. */
    std.graphics.Graphics.prototype.createGUI = function(name, url, width, height) {
        if (width && height)
            return new std.graphics.GUI(this._simulator.invoke("createWindowFile", name, url, width, height));
        else
            return new std.graphics.GUI(this._simulator.invoke("createWindowFile", name, url));
    };

    /** Request that the given URL be added as a module in the UI. */
    std.graphics.Graphics.prototype.addGUIModule = function(name, url) {
    	system.print("adding GUI module");
    	return new std.graphics.GUI(this._simulator.invoke("addModuleToUI", name, url));
    };
    
    /** Request that the given URL be presented as a widget. */
    std.graphics.Graphics.prototype.createBrowser = function(name, url) {
        return new std.graphics.GUI(this._simulator.invoke("createWindow", name, url));
    };

    /** Get basic camera description. This is read-only data. */
    std.graphics.Graphics.prototype.camera = function() {
        return this.invoke("camera");
    };

    /** Compute a vector indicating the direction a particular point in the
     *  camera's viewport points. (x,y) should be in the range (-1,-1)-(1,1),
     *  the same as the values provided by mouse click and drag events.  If no
     *  arguments are passed, this returns the central direction, i.e. the same
     *  as cameraDirection(0,0).
     */
    std.graphics.Graphics.prototype.cameraDirection = function(x, y) {
        var orient = this.presence.getOrientation();

        if (!x && !y)
            return orient.zAxis().neg();

        var cam = this.invoke("camera");
        var xRadian = util.sin(cam.fov.x * .5) * x;
        var yRadian = util.sin(cam.fov.y * .5) * y;

        var zpart = orient.zAxis().neg().mul( util.cos(cam.fov.y * .5) );
        var xpart = orient.xAxis().mul(xRadian);
        var ypart = orient.yAxis().mul(yRadian);
        var dir = xpart.add(ypart).add(zpart);
        return dir;
    };

})();

// Import additional utilities that anybody using this class will need.
system.require('input.em');
