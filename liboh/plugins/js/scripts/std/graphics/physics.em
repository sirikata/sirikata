/*  Sirikata
 *  physics.em
 *
 *  Copyright (c) 2011, Stanford University
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

system.require('std/movement/movableremote.em');

/** The PhysicsProperties class allows you to modify the properties of a physics object from a UI. */
std.graphics.PhysicsProperties = system.Class.extend(
    {
        init: function(sim, init_cb) {
            this._sim = sim;

            this._selected = undefined;

            this._sim.addGUIModule(
                "PhysicsSettings", "scripting/physics.js",
                std.core.bind(function(physics_gui) {
                                  this._ui = physics_gui;
                                  this._ui.bind("requestPhysicsUpdate", std.core.bind(this.requestPhysicsUpdate, this));
                                  if (init_cb) init_cb();
                              }, this)
            );
        },

        toggle: function() {
            this._ui.call('PhysicsSettings.toggleVisible');
        },

        update: function(vis) {
            this._selected = vis;
            if (this._selected) {
                var phy = this._selected.physics;
                this._ui.call('PhysicsSettings.setUIInfo', phy.treatment || 'ignore', phy.bounds || 'sphere', phy.mass || 1.0);
            }
            else {
                this._ui.call('PhysicsSettings.disable');
            }
        },

        requestPhysicsUpdate: function(treatment, collision_mesh, mass) {
            var physics_settings = {
                treatment : treatment,
                bounds : collision_mesh,
                mass : mass
            };
            (new std.movement.MovableRemote(this._selected)).setPhysics(physics_settings);
        }
    }
);
