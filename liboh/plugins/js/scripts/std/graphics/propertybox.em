/*  Sirikata
 *  propertybox.em
 *
 *  Copyright (c) 2011, Kotaro Ishiguro
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

 system.require('std/core/bind.em');
 system.require('std/core/repeatingTimer.em');

if (typeof(std) === "undefined") std = {};
if (typeof(std.propertybox) === "undefined") std.propertybox = {};

(
function () {
    var pb = std.propertybox;

    pb.PropertyBox = function(parent) {
        this._parent = parent;
    try {
            var propertybox_gui = this._parent._simulator.addGUIModule("propertybox", "propertybox/propertybox.js");
            this._propertyWindow = propertybox_gui;
            this._self = system.presences[0];
            this._selected_visible = this._self;
            // Magic number of 0.5 seconds for updating the property box automatically
            this._updateTimer = new std.core.RepeatingTimer(.5, std.core.bind(this.HandleUpdateProperties, this));
        } catch (ex) {
            system.print(ex);
        }
    };

    pb.PropertyBox.prototype.HandleUpdateProperties = function(visible) {
        if (visible) {
            // Remember the last object selected
            this._selected_visible = visible;
        } else if (this._selected_visible) {
            // If nothing is selected, show properties for the last object selected
            visible = this._selected_visible;
        } else {
            return; // Shouldn't ever reach here
        }
        var property_pos = visible.getPosition();
        var property_vel = visible.getVelocity();
        var property_o   = visible.getOrientation();
        var property_o_vel = visible.getOrientationVel();
        var meshname = "'" + visible.getMesh() + "'";
        var scale = visible.getScale();
        var property_id;
        if (visible == this._self)
        {
            property_id = "'" + visible.getPresenceID() + "'";
        } else {
            property_id = "'" + visible.getVisibleID() + "'";
        }
        var self_pos = this._self.getPosition();
        var distance = property_pos.sub(self_pos).length();
        this._propertyWindow.eval(
            "updateProperties(" + property_pos['x'] + ","
                                + property_pos['y'] + ","
                                + property_pos['z'] + ","
                                + property_vel['x'] + ","
                                + property_vel['y'] + ","
                                + property_vel['z'] + ","
                                + property_o['x'] + ","
                                + property_o['y'] + ","
                                + property_o['z'] + ","
                                + property_o['w'] + ","
                                + property_o_vel['x'] + ","
                                + property_o_vel['y'] + ","
                                + property_o_vel['z'] + ","
                                + property_o_vel['w'] + ","
                                + scale + ","
                                + distance + ","
                                + meshname + ","
                                + property_id + ");");

    };

    pb.PropertyBox.prototype.TogglePropertyBox = function() {
        // If open, close it. If closed, open it.
        this._propertyWindow.eval("if ($(\"#property-box\").dialog('isOpen')) $(\"#property-box\").dialog('close'); else $(\"#property-box\").dialog('open');");
    };

})();
