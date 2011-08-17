system.require('std/core/bind.em');

if (typeof(std) === "undefined") std = {};
if (typeof(std.ezui) === "undefined") std.ezui = {};


(
function () {

    var ui = std.ezui;
    var me;

    ui.EZUI = function(parent, init_cb) {
        me = this;
        this._parent = parent;
        this._handleUiEval << [{'uieval'::}]
        try {
            this._initOrResetUI(init_cb);
            this._viewingAvatar = system.self;
            this._selected_visible = this._self;
        } catch (ex) {
            system.print(ex);
        }
        this.x = 150;
        this.y = 150;
    };


    ui.EZUI.prototype.onReset = function(reset_cb) {
        this._initOrResetUI(reset_cb);
    };

    ui.EZUI.prototype._initOrResetUI = function(init_cb) {
        var ezui_gui = this._parent._simulator.addGUIModule(
            "ezui", "ezui/ezui.js",
            std.core.bind(function(gui) {
                            me._ezuiWindow = gui;
                            handleSendMessageToController = function(msg) {
                                if (msg) {
                                    msg.vis = me._selected_visible.toString();
                                    msg >> me._selected_visible >> [];
                                }
                            };
                            handleSendMessageToViewer = function(msg) {
                                if (msg) msg >> me._viewingAvatar >> [];
                            };
                            handleSendMessage = function(obj) {
                                if (obj && pres) obj.msg >> obj.pres >> [];
                            }
                            uipp = function(msg) {
                                system.print(std.core.serialize(msg.obj)+'\n');
                            }
                            me._ezuiWindow.bind("sendMsgController", handleSendMessageToController);
                            me._ezuiWindow.bind("sendMsgViewer", handleSendMessageToViewer);
                            me._ezuiWindow.bind("sendMsg", handleSendMessage);
                            me._ezuiWindow.bind("pretty", uipp);
                            if (init_cb) init_cb()
                        }, this)
        );
    };
    
    /* When we get the script from the selected visible, set up and display the GUI. */
    
    ui.EZUI.prototype.onRequestResponse = function(respmsg) {
        me._ezuiWindow.guiEval("__reset();");
        function ezui_escape(s) {
            for (var i = 0; i < s.length; i++) {
                if (s.charAt(i) == "'" || s.charAt(i) == "\"") {
                    var beginning = s.slice(0, i);
                    var end = s.slice(i);
                    s = beginning+"\\"+end;
                    i = i + 1;
                } else if (s.charAt(i) == "\n") {
                    var beginning = s.slice(0, i);
                    var end = s.slice(i);
                    s = beginning+"\\"+end;
                    i = i + 1;
                }
            }
            return s;
        }
        if (respmsg.script) {
            me._selected_visible = respmsg.self;
            var av = me._viewingAvatar;
            var pos = av.getPosition();
            var vel = av.getVelocity();
            var ori = av.getOrientation();
            var orivel = av.getOrientationVel();
            var mesh = av.getMesh();
            me._ezuiWindow.guiEval("__set_viewer('"+av.toString()+"');");
            me._ezuiWindow.guiEval("__set_controller('"+me._selected_visible.toString()+"');");
            if (typeof(respmsg.userData) !== 'undefined') {
                for (p in respmsg.userData) {
                    if (respmsg.userData.hasOwnProperty(p)) {
                        me._ezuiWindow.guiEval("_('"+p+"', "+std.core.serialize(respmsg.userData[p])+");");
                    }
                }
            }
            if (typeof(respmsg.globalData) !== 'undefined') {
                for (p in respmsg.globalData) {
                    if (respmsg.globalData.hasOwnProperty(p)) {
                        me._ezuiWindow.guiEval("__set_global_data('"+p+"', "+std.core.serialize(respmsg.globalData[p])+");");
                    }
                }
            }
            var command = "__init_viewer("+pos.x+","+pos.y+","+pos.z+","+
                                        vel.x+","+vel.y+","+vel.z+","+
                                        ori.x+","+ori.y+","+ori.z+","+ori.w+","+
                                        orivel.x+","+orivel.y+","+orivel.z+","+orivel.w+",'"+
                                        mesh+"','"+av+"')";
            me._ezuiWindow.guiEval(command);
            me._ezuiWindow.guiEval("__move_to_picked_position("+me.x+","+me.y+");");
            //command = "__script('"+ezui_escape(respmsg.script)+"');";
            me._ezuiWindow.guiEval(respmsg.script);
            //me._ezuiWindow.guiEval(command);
        }
        if (!respmsg.script || respmsg.script == "") {
            //no ui for this visible
            me.hide();
            return;
        }
        
        //if (respmsg.userSetupFunction != "" && respmsg.userData) {
        //    me._ezuiWindow.guiEval(respmsg.userSetupFunction + "("+system.pretty(respmsg.userData)+");");
        //}
        //me._ezuiWindow.guiEval("$('#ezui_ui').dialog('open');");
    };
    
    ui.EZUI.prototype.onNoResponse = function() {
        system.print("[EZUI] got no response from selected visible.\n");
        me.hide();
    };
    
    ui.EZUI.prototype.handleUpdateUI = function(visible) {
        if (visible) {
            // Remember the last object selected
            //this._selected_visible = visible;
        } else {
            return;
        }
        
        var request = {};
        request.ezuiRequest = 0; //this is the property that ezuiEntity.em is looking for
        request.sender = ""+this._viewingAvatar;
        request.recipient = ""+visible;
        request >> visible >> [this.onRequestResponse, 5, this.onNoResponse];
    };


    ui.EZUI.prototype.show = function(visible, x, y) {
        this.x = x;
        this.y = y;
        if (visible) {
            // if we're displaying a UI for one presence and the user clicks on something else,
            // call __handle_ui_will_close so the UI can sync with its controller before it's dismissed.
            if (this._selected_visible && visible.toString() != this._selected_visible.toString()) this._ezuiWindow.guiEval("__handle_ui_will_close();");
            this.handleUpdateUI(visible);
        } else {
            this.hide();
        }
    };
    
    ui.EZUI.prototype.hide = function() {
        this._ezuiWindow.guiEval("close();");
        //this._ezuiWindow.guiEval("$(\"#ezui_ui\").dialog('close');");
    };
    
    ui.EZUI.prototype._handleUiEval = function(msg) {
        me._ezuiWindow.guiEval(msg.uieval);
    }
    
    /*
    ezui_handleUserDataChanged = function(msg) {
        var data = msg.data
        this._ezuiWindow.guiEval("__handle_user_data_changed("+std.core.pretty(data)+");");
    }*/
    
    ezui_handleGlobalDataChanged = function(msg, sender) {
        if (sender.toString() != me._selected_visible) return;
        var d = msg.updatedGlobalData;
        var cmd = "__handle_global_data_changed("+std.core.serialize(d)+");";
        me._ezuiWindow.guiEval(cmd);
    }
    ezui_handleGlobalDataChanged << [{'updatedGlobalData'::}];

})();