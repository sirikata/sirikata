// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

system.require("std/library.em");
system.require("std/script/scripter.em");

/** The PresList class brings up a UI for scripting and seeing 
 * already scripted and visible presences.
 */
std.graphics.PresenceList = system.Class.extend(
    {
        init: function(pres, sim, scripter) {
            this._pres = pres;
            this._sim = sim;
	    this._scripter = scripter;
	    
	    this.scriptedObj = [];
	    this.visibleObj = [];
            
            this._sim.addGUIModule(
                "PresenceList", "scripting/presenceList.js",
                std.core.bind(
                    function(presList_gui) {
                        this._ui = presList_gui;
                        this._ui.bind("scriptSelected", std.core.bind(this.onScriptRequest, this));
			this._ui.bind("defaultTest", std.core.bind(this.onDefaultTest, this));
			//  this._ui.bind("clearSelected", std.core.bind(this.onClearSelected, this));
			this._ui.bind("clearAll", std.core.bind(this.onClearAll, this));

		        this.addObject(this._pres,"Visible");
		    },
                    this));

	    //	this._pres.onProxAdded(std.core.bind(this.proxAddedCallback, this));
	    //	this._pres.onProxRemoved(std.core.bind(this.proxRemovedCallback, this));
	},

		toggle: function() {
            this._ui.call('PresenceList.toggleVisible');
		},

		includesObject: function(objid,list){
			for (var i = 0; i < list.length; i++) {
				if (list[i] === objid) {
					return i;
				}
			}
			return null;
		},
		
		//adds object to list
		addObject: function(new_obj, listName) {
			var list = undefined;
			switch(listName.length){
				case 7: list = this.visibleObj;
				case 8: list = this.scriptedObj;
			}
			if (!this.includesObject(new_obj,list)) {
				var index = list.push(new_obj) - 1;
				this._ui.call('PresenceList.addElem',new_obj.toString());
			}
		},
		
		//sends request to scripter
		onScriptRequest: function(obj_index) {
			if (obj_index){
				var obj = this.scriptedObj[obj_index];
				this._scripter.script(obj);
			}
		},
		
		//clears array of scripted objects
		onClearAll: function() {
			this.scriptedObj.length = 0;
		},
		
		onDefaultTest: function(message) {
			system.print("\n\n test message: " + message + " \n\n");
		}/*,
		
		onClearSelected: function(index) {
			this.scriptedObj.splice(index,1);
		}, 
	
		proxAddedCallback: function(new_addr_obj) {
            system.print("\n\n add visible \n\n");
		}, 
		
		//prox event: removes address from vis list 
        proxRemovedCallback: function(old_addr_obj) {
			if(system.self.toString() == new_addr_obj.toString())
                return;
			system.print("\n\n removed visible\n\n");
		}
	*/
    }
);
