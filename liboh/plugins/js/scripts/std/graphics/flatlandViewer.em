system.require('std/core/bind.em');

if (typeof(std) === "undefined") std = {};
if (typeof(std.fl) === "undefined") std.fl = {};


(
function () {

    var ui = std.fl;
    var me;

    ui.FL = function(parent, init_cb) {
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


    ui.FL.prototype.onReset = function(reset_cb) {
        this._initOrResetUI(reset_cb);
    };

    ui.FL.prototype._initOrResetUI = function(init_cb) {
        var fl_gui = this._parent._simulator.addGUIModule(
            "fl", "flatland/flatland.js",
            std.core.bind(function(gui) {
                            me._flWindow = gui;
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
                            me._flWindow.bind("sendMsgController", handleSendMessageToController);
                            me._flWindow.bind("sendMsgViewer", handleSendMessageToViewer);
                            me._flWindow.bind("sendMsg", handleSendMessage);
                            me._flWindow.bind("pretty", uipp);
                            if (init_cb) init_cb()
                        }, this)
        );
    };
    
    /* When we get the script from the selected visible, set up and display the GUI. */
    
    ui.FL.prototype.onRequestResponse = function(respmsg) {
        me._flWindow.guiEval("__reset();");
        function fl_escape(s) {
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
            me._flWindow.guiEval("__set_viewer('"+av.toString()+"');");
            me._flWindow.guiEval("__set_controller('"+me._selected_visible.toString()+"');");
            if (typeof(respmsg.userData) !== 'undefined') {
                for (p in respmsg.userData) {
                    if (respmsg.userData.hasOwnProperty(p)) {
                        me._flWindow.guiEval("_('"+p+"', "+std.core.serialize(respmsg.userData[p])+");");
                    }
                }
            }
            if (typeof(respmsg.globalData) !== 'undefined') {
                for (p in respmsg.globalData) {
                    if (respmsg.globalData.hasOwnProperty(p)) {
                        me._flWindow.guiEval("__set_global_data('"+p+"', "+std.core.serialize(respmsg.globalData[p])+");");
                    }
                }
            }
            var command = "__init_viewer("+pos.x+","+pos.y+","+pos.z+","+
                                        vel.x+","+vel.y+","+vel.z+","+
                                        ori.x+","+ori.y+","+ori.z+","+ori.w+","+
                                        orivel.x+","+orivel.y+","+orivel.z+","+orivel.w+",'"+
                                        mesh+"','"+av+"')";
            me._flWindow.guiEval(command);
            me._flWindow.guiEval("__move_to_picked_position("+me.x+","+me.y+");");
            //command = "__script('"+fl_escape(respmsg.script)+"');";
            me._flWindow.guiEval(respmsg.script);
            //me._flWindow.guiEval(command);
        }
        if (!respmsg.script || respmsg.script == "") {
            //no ui for this visible
            me.hide();
            return;
        }
        
        //if (respmsg.userSetupFunction != "" && respmsg.userData) {
        //    me._flWindow.guiEval(respmsg.userSetupFunction + "("+system.pretty(respmsg.userData)+");");
        //}
        //me._flWindow.guiEval("$('#fl_ui').dialog('open');");
    };
    
    ui.FL.prototype.onNoResponse = function() {
        system.print("[FL] got no response from selected visible.\n");
        me.hide();
    };
    
    ui.FL.prototype.handleUpdateUI = function(visible) {
        if (visible) {
            // Remember the last object selected
            //this._selected_visible = visible;
        } else {
            return;
        }
        
        var request = {};
        request.flRequest = 0; //this is the property that flEntity.em is looking for
        request.sender = ""+this._viewingAvatar;
        request.recipient = ""+visible;
        request >> visible >> [this.onRequestResponse, 5, this.onNoResponse];
    };


    ui.FL.prototype.show = function(visible, x, y) {
        this.x = x;
        this.y = y;
        if (visible) {
            // if we're displaying a UI for one presence and the user clicks on something else,
            // call __handle_ui_will_close so the UI can sync with its controller before it's dismissed.
            if (this._selected_visible && visible.toString() != this._selected_visible.toString()) this._flWindow.guiEval("__handle_ui_will_close();");
            this.handleUpdateUI(visible);
        } else {
            this.hide();
        }
    };
    
    ui.FL.prototype.hide = function() {
        this._flWindow.guiEval("close();");
        //this._flWindow.guiEval("$(\"#fl_ui\").dialog('close');");
    };
    
    ui.FL.prototype._handleUiEval = function(msg) {
        me._flWindow.guiEval(msg.uieval);
    }
    
    /*
    fl_handleUserDataChanged = function(msg) {
        var data = msg.data
        this._flWindow.guiEval("__handle_user_data_changed("+std.core.pretty(data)+");");
    }*/
    
    fl_handleGlobalDataChanged = function(msg, sender) {
        if (sender.toString() != me._selected_visible) return;
        var d = msg.updatedGlobalData;
        var cmd = "__handle_global_data_changed("+std.core.serialize(d)+");";
        me._flWindow.guiEval(cmd);
    }
    fl_handleGlobalDataChanged << [{'updatedGlobalData'::}];

})();