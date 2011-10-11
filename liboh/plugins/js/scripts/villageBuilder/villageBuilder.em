/*  Sirikata
 *  villageBuilder.em
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

var facilities = [];
var villageBuilder_vg = null;
var villageBuilder_sp = null;
var villageBuilder_st = null;

std.graphics.VillageBuilder = system.Class.extend(
    {
        init: function(sim, init_cb) {
            this._sim = sim;
            this._selected = undefined;

            this._sim.addGUIModule(
                "VillageBuilder", 'scripting/villageBuilder.js',
                std.core.bind(function(villageBuilder_gui) {
                  this._ui = villageBuilder_gui;
                  this._ui.bind("village_init", std.core.bind(this.draw_village_land, this));
                  this._ui.bind("load_village_street_template",std.core.bind(this.load_village_street_template, this));
                  this._ui.bind("extend_village_streets", std.core.bind(this.extend_village_streets, this));
                  this._ui.bind("erase_village_street", std.core.bind(this.erase_village_street, this));
                  this._ui.bind("draw_village_streets", std.core.bind(this.draw_village_streets, this));
                  this._ui.bind("draw_village_facility", std.core.bind(this.draw_village_facility, this));
                  this._ui.bind("place_car",std.core.bind(this.place_car, this));
                  if (init_cb) init_cb();
				}, this)
            );
        },

        onReset : function(reset_cb) {
            this._sim.addGUIModule(
                "VillageBuilder", "scripting/villageBuilder.js",
                std.core.bind(function(chat_gui) {
                                  if (reset_cb) reset_cb();
                              }, this));
        },

        toggle: function() {
		    this._ui.call('VillageBuilder.toggleVisible');
        },

        callback_func: function(vertexGrid,space) {
            system.import('villageBuilder/streetTools.em');
            villageBuilder_vg = vertexGrid;
            villageBuilder_sp = space;
            villageBuilder_st = createEmptyStreet();
        },

        draw_village_land: function(createPosition, cols, rows) {
            system.import('villageBuilder/vertexGridGenerator-standalone.em');
            var islandPos = <createPosition.x,createPosition.y,createPosition.z>;
            createVertexGrid(islandPos, cols, rows, 12, this.callback_func);
        },

        place_car: function() {
            system.import('villageBuilder/carGenerator.em');
            createCar(villageBuilder_st, villageBuilder_sp);
        },

        load_village_street_template: function(template) {
            system.__debugPrint("\n\nLOADING STREET TEMPLATE...\n\n");
            
            system.import('villageBuilder/streetTools.em');
            var st = createTemplate(villageBuilder_st,template, villageBuilder_vg);
            system.__debugPrint("\n >> # new street edges created: "+st.length+"\n"); 
            for(var i = 0; i < st.length; i++) {
                this._ui.call('VillageBuilder.canvas_drawstreet',st[i]);
            }
        
        },

        erase_village_street: function(c1,r1,c2,r2) {
            system.import('villageBuilder/streetTools.em');
            eraseStreet(villageBuilder_st, villageBuilder_vg[c1][r1], villageBuilder_vg[c2][r2]);
        },

        extend_village_streets: function(col1,row1,col2,row2) {
            system.import('villageBuilder/streetTools.em');
            extendStreet(villageBuilder_st, villageBuilder_vg[col1][row1], villageBuilder_vg[col2][row2]);
        },

        draw_village_streets: function() {
            system.__debugPrint("\n\n========DRAW_VILLAGE_STREETS===========\n\n");
            system.import('villageBuilder/streetGenerator-standalone.em');
            system.__debugPrint("\n >> villageBuilder_st has "+villageBuilder_st.nodes.length+" nodes and "+villageBuilder_st.edges.length+" edges!");
            placeStreet(villageBuilder_st, villageBuilder_sp);
        },
        
        draw_village_facility: function(facilityType, orientation, col, row) {
            system.__debugPrint("\n\n========DRAW_VILLAGE_FACILITY=========\n\n");
            var x = villageBuilder_vg[col][row].x + villageBuilder_sp*0.1;
            var y = villageBuilder_vg[col][row].y;
            var z = villageBuilder_vg[col][row].z + villageBuilder_sp*0.1;
            var anchor = <x,y,z>;
            system.import('villageBuilder/facilityGenerator.em');
            if(facilityType == "school") var space = villageBuilder_sp*1.8;
            else var space = villageBuilder_sp*0.8;
            createFacility(facilityType, orientation, anchor, space); // multiplies space by 0.8 to make facility fit in lot
        }
    }
);

