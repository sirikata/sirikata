/*  Sirikata
 *  setMesh.em
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


/** The SetMesh class allows the user to change the mesh of any presence in the world or create a new presence 
 * with a chosen mesh.  The list of meshes automatically adds any meshes added to open3dhub.com.
 */
std.graphics.SetMesh = system.Class.extend(
    {
        init: function(sim, init_cb) {
            this._sim = sim;

            this._selected = undefined;

            this._sim.addGUIModule(
                "SetMesh", "scripting/setMesh.js",
                std.core.bind(function(setMesh_gui) {
                                  this._ui = setMesh_gui;
                                  this._ui.bind("setmesh", std.core.bind(this.handleSetMesh, this));
                                  this._ui.bind("createNewPres", std.core.bind(this.handleCreateNew, this));
                                  
                                  if (init_cb) init_cb();
                              }, this)
            );
        },

        onReset : function(reset_cb) {
            this._sim.addGUIModule(
                "SetMesh", "scripting/setMesh.js",
                std.core.bind(function(chat_gui) {
                                  if (reset_cb) reset_cb();
                              }, this));
        },

        toggle: function() {
            this._ui.call('SetMesh.toggleVisible');
        },
        
    handleSetMesh: function(meshurl) {				
		var scriptToSend = 'system.self.mesh = \"' + meshurl + '\"';
		var msgHandler = {request:'script', script: scriptToSend
		};
			
		if (simulator._selected != null) {
			//system.__debugPrint('\nSomething is changing\n');
			//system.__debugPrint(simulator._selected.toString());
			msgHandler >> simulator._selected >> [];
		}
    },

        handleCreateNew: function(meshurl) {
            function callbackNew(newPres) {
		//system.__debugPrint('created!!!');
		    }
            var pos = system.self.getPosition();
            pos.x = pos.x - 3;
            system.createPresence(meshurl, callbackNew, pos);
        }
        
        
    }
);
