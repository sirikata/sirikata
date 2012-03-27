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
system.require('std/escape.em');
system.require('std/graphics/animationInfo.em');


(
function() {

    var ns = std.graphics;

    /** @namespace
     * The Graphics class wraps the underlying graphics simulation,
     *  allowing you to get access to input, control display options,
     *  and perform operations like picking in response to mouse
     *  clicks.
     *
     *  The callback passed in is invoked when Ogre has finished
     *  initializing.
     */
    std.graphics.Graphics = function(pres, name, cb, reset_cb) {

        if (typeof(reInitialize) === 'unefined')
            reInitialize = false;
        
        this.presence = pres;
        this._cameraMode = 'first';
        this._simulator = pres.runSimulation(name);

        //if (!reInitialize)
        if (!this._isReady())
        {
            this.inputHandler = new std.graphics.InputHandler(this);
            this._setOnReady(cb, reset_cb);
            this._animationInfo = new std.graphics.AnimationInfo(pres, this);
        }
        else
        {
            this.inputHandler = new std.graphics.InputHandler(this);
            this._animationInfo = new std.graphics.AnimationInfo(pres, this);
            system.event(std.core.bind(this._handleOnReady,this,cb,true));
        }

        this._guis = {};
    };

    std.graphics.Graphics.prototype.invoke = function() {
        // Just forward manual invoke commands directly
        return this._simulator.invoke.apply(this._simulator, arguments);
    };

    /**
     @returns true if already have a viewer for this presence.  false otherwise.
     */
    std.graphics.Graphics.prototype._isReady = function()
    {
        return this.invoke('isReady');
    };


    /**
     @param {boolean} alreadyInitialized is whether a viewer was already created for this
     presence, or whether its the first one brought up.  (True if a
     viewer had already been created, false otherwise.)
     */
    std.graphics.Graphics.prototype._handleOnReady = function(cb,alreadyInitialized)
    {
        // Reinitialize camera mode
        this.invoke('onTick', std.core.bind(this._onTick, this));
        if (cb) cb(this,alreadyInitialized);
    };

    /** Set the callback to invoke when the system is ready for rendering. */
    std.graphics.Graphics.prototype._setOnReady = function(cb, reset_cb) {
        this.invoke('onReady',
                    std.core.bind(this._handleOnReady, this, cb,false),
                    std.core.bind(this._handleOnReady, this, reset_cb)
                   );
    };

    /** Set a callback to be invoked once before each frame.
     */
    std.graphics.Graphics.prototype.onTick = function(cb) {
        this._onTickCB = cb;
    };

    /** Internal onTick handler.
     *  @private
     */
    std.graphics.Graphics.prototype._onTick = function(t, dt) {
        if (this._onTickCB) this._onTickCB(t, dt);
    };

    /** Hides the loading screen. Call once you're GUIs and basic
     * loading is done.
     */
    std.graphics.Graphics.prototype.hideLoadScreen = function() {
        this.invoke('evalInUI', 'hideLoadScreen()');
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
    
    std.graphics.Graphics.prototype.getAnimationList = function(pres) {
      return this.invoke('getAnimationList', pres);
    };

    std.graphics.Graphics.prototype.startAnimation = function(vis, anim_name, responseToMsg) {
      this.invoke('startAnimation', vis, anim_name);
  
      if (!responseToMsg)
        this._animationInfo.sendAnimationInfo("AnimationInfo", vis, anim_name);
    };

    std.graphics.Graphics.prototype.stopAnimation = function(vis, responseToMsg) {
      this.invoke('stopAnimation', vis);

      if (!responseToMsg)
        this._animationInfo.sendAnimationInfo("AnimationInfo", vis, "");
    };



    /** Request that the OH shut itself down, i.e. that the entire application exit. */
    std.graphics.Graphics.prototype.quit = function() {
        this.invoke('quit');
    };

    /** Request a screenshot be taken and stored on disk. */
    std.graphics.Graphics.prototype.screenshot = function() {
        this.invoke('screenshot');
    };

    /** Pick at the given location for an object. Stores the hit point which can be retrieved with pickedPosition(). 
     *  @param {number} x the x-coorindate on screen
     *  @param {number} y the y-coordinate on screen
     *  @param {visible|presence|bool} ignore indicates whether to ignore some
     *  object, either by specifying the object or if a bool is provided, it
     *  indicates the presence that allocated the graphics object should be
     *  ignored.
     */
    std.graphics.Graphics.prototype.pick = function(x, y, ignore) {
        var result = this.invoke('pick', x, y, ignore);
        if (result.object) {
            this._pickedPosition = new util.Vec3(result.position.x, result.position.y, result.position.z);
            return result.object;
        }
        else {
            this._pickedPosition = undefined;
            return undefined;
        }
    };

    /** */
    std.graphics.Graphics.prototype.pickedPosition = function() {
        return this._pickedPosition;
    };

    /** Request the bounding box for the object be enabled or disabled. */
    std.graphics.Graphics.prototype.bbox = function(obj, on) {
        return this.invoke('bbox', obj, on);
    };

    /** Request that the given URL be presented as a widget. */
    std.graphics.Graphics.prototype.createGUI = function(name, url, width, height, cb) {
        if (width && height)
            return new std.graphics.GUI(name, this._simulator.invoke("createWindowFile", name, url, width, height), cb);
        else
            return new std.graphics.GUI(name, this._simulator.invoke("createWindowFile", name, url), cb);
    };

    /** Request that the given URL be added as a module in the UI. */
    std.graphics.Graphics.prototype.addGUIModule = function(name, url, cb) {
        this._guis[name] = new std.graphics.GUI(name, this._simulator.invoke("addModuleToUI", name, url), cb);
    	return this._guis[name];
    };

    /** Request that the given script text be added as a module in the UI. */
    std.graphics.Graphics.prototype.addGUITextModule = function(name, js_text, cb) {
        // addTextModuleToUI expects pre-escaped strings
        js_text = Escape.escapeString(js_text);
        this._guis[name] = new std.graphics.GUI(name, this._simulator.invoke("addTextModuleToUI", name, js_text), cb);
    	return this._guis[name];
    };

    /** Get the GUI for a previously loaded GUI module. */
    std.graphics.Graphics.prototype.getGUIModule = function(name) {
    	return this._guis[name];
    };

    /** Request that the given URL be presented as a widget. */
    std.graphics.Graphics.prototype.createBrowser = function(name, url, cb) {
        return new std.graphics.GUI(name, this._simulator.invoke("createWindow", name, url), cb);
    };

    /** Get basic camera description. This is read-only data. */
    std.graphics.Graphics.prototype.camera = function() {
        return this.invoke("camera");
    };

    /** Gets the camera's current position. Note that this can be different from
     *  the position of the presence it is attached to because it may be in 3rd
     *  person mode. Useful if you need to figure out something based on the
     *  display, e.g. how a click on a particular x,y screen coordinate should
     *  map to the world.
     */
    std.graphics.Graphics.prototype.cameraPosition = function() {
        var cam_info = this.camera();
        return < cam_info.position.x, cam_info.position.y, cam_info.position.z >;
    };

    /** Gets the camera's current orientation. Note that this can be different from
     *  the position of the presence it is attached to because it may be in 3rd
     *  person mode. Useful if you need to figure out something based on the
     *  display, e.g. how a click on a particular x,y screen coordinate should
     *  map to the world.
     */
    std.graphics.Graphics.prototype.cameraOrientation = function() {
        var cam_info = this.camera();
        return new util.Quaternion(cam_info.orientation.x, cam_info.orientation.y, cam_info.orientation.z, cam_info.orientation.w);
    };

    /** Compute a vector indicating the direction a particular point in the
     *  camera's viewport points. (x,y) should be in the range (-1,-1)-(1,1),
     *  the same as the values provided by mouse click and drag events.  If no
     *  arguments are passed, this returns the central direction, i.e. the same
     *  as cameraDirection(0,0).
     */
    std.graphics.Graphics.prototype.cameraDirection = function(x, y) {
        var orient = this.cameraOrientation();

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

    /** Marks an object as visible or invisible. */
    std.graphics.Graphics.prototype.visible = function(obj, setting) {
        this.invoke('visible', obj, setting);
    }

    /** Set the camera's position. */
    std.graphics.Graphics.prototype.setCameraPosition = function(pos) {
        this.invoke('setCameraPosition', pos.x, pos.y, pos.z);
    };

    /** Set the camera's orientation. */
    std.graphics.Graphics.prototype.setCameraOrientation = function(orient) {
        this.invoke('setCameraOrientation', orient.x, orient.y, orient.z, orient.w);
    };
    
    std.graphics.Graphics.prototype.axis = function(obj, axis, visible, glbl) {
        this.invoke('axis', obj, axis.toLowerCase(), visible, glbl);
    };
    
    std.graphics.Graphics.prototype.axes = function(obj, visible, glbl) {
        this.invoke('axis', obj, 'x', visible, glbl);
        this.invoke('axis', obj, 'y', visible, glbl);
        this.invoke('axis', obj, 'z', visible, glbl);
    };
    
    /* convert world coordinates to screen coordinates, return a {x: , y: } object */
    std.graphics.Graphics.prototype.world2Screen = function(p) {
        var result = this.invoke('world2Screen', p.x, p.y, p.z);
        if (result) {
            return result;
        } else {
            return undefined;
        }
    };
    
    std.graphics.Graphics.prototype.newDrawing = function(drawingName, obj, inheritOrient, inheritScale, visible) {
        this.invoke("newDrawing", drawingName, obj, inheritOrient, inheritScale, visible);
    };
    
    std.graphics.Graphics.prototype.setMat = function(er, eg, eb, dr, dg, db, sr, sg, sb, ar, ag, ab) {
        return this.invoke("setMat", er, eg, eb, dr, dg, db, sr, sg, sb, ar, ag, ab);
    };
    
    std.graphics.Graphics.prototype.setInheritOrient = function(drawingName, inheritOrient) {
        this.invoke("setInheritOrient", drawingName, inheritOrient);
    };
    
    std.graphics.Graphics.prototype.setInheritScale = function(drawingName, inheritScale) {
        this.invoke("setInheritScale", drawingName, inheritScale);
    };
    
    std.graphics.Graphics.prototype.setVisible = function(drawingName, visible) {
        this.invoke("setVisible", drawingName, visible);
    };
    
    std.graphics.Graphics.prototype.shape = function(drawingName, clear, type, points) {
        clear = clear ? true : false;
        var args = ["shape", drawingName, clear];
        if (typeof(type) === "number") {
            args.push(type);
        }
        
        if (points) {
            for (var i = 0; i < points.length; i++) {
                args.push(points[i].x);
                args.push(points[i].y);
                args.push(points[i].z);
            }
        }
    
        this.invoke.apply(this, args);
    };

    /** Set the maximum number of objects that will be
     *  displayed. Objects are prioritized intelligently, so nearer &
     *  larger objects are displayed instead of distant & small ones.
     */
    std.graphics.Graphics.prototype.setMaxObjects = function(maxobjs) {
        this.invoke("setMaxObjects", maxobjs, visible);
    };

    /** Sets the skybox or disables it.
     *  \param type 'disabled', 'cube', 'dome', or 'plane'
     *  \param img the image URL to use
     *  \param distance distance to box
     *  \param tiling number of times to tile for 'dome'
     *  \param curvature curvature for the dome
     *  \param orientation quaternion specifying rotation of the box
     */
    std.graphics.Graphics.prototype.skybox = function(type, img, distance, tiling, curvature, orientation) {
        // We need to only pass in the used arguments. Find the first undefined value and chop off at that point.
        args = ['setSkybox', type, img, distance, tiling, curvature];
        if (orientation)
            args.splice(-1, 0, orientation.x, orientation.y, orientation.z, orientation.w);
        var arg_undefined_idx = args.indexOf(undefined);
        if (arg_undefined_idx != -1)
            args = args.slice(0, arg_undefined_idx);
        this.invoke.apply(this, args);
    };

})();

// Import additional utilities that anybody using this class will need.
system.require('input.em');
