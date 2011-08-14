/*
  Interface For Scripts:
    write(html) - writes the html to the ui
    button(text, callback, [id]) - writes a button with the given text and callback to the ui. Custom id is optional.  Callback takes button text as param.
            Note that the callback function is a js function in the UI script, not an Emerson function on the controller entity.
    clear() - clears the ui of all text and buttons
    _(varname) - returns the user data variable with the given name.
    _(varname, newvalue) - sets the user data variable named varname to newvalue and returns the new value.
    _() - returns the user data object
    GLOBAL_(varname) - returns the global data variable with the given name.
    GLOBAL_(varname, newvalue) - sets the global data variable named varname to newvalue and returns the new value.
    GLOBAL_() - returns the global data object
    G_(...) or __(...) - shorthand for GLOBAL_(...)
    onUiWillClose(func) - sets func as the function to be called when the UI is about to be closed by the user.
    
    syncData() - saves all data (user and global) to the controller presence.
    sendToController(msg) - sends the message object to the controller presence.
    sendToViewer(msg) - sends the message object to the avatar viewing the ui.
    setReceivedMessageCallback(func) - sets func as the function to be called when the ui gets a message from the controller presence.


*/


sirikata.ui(
    'ezui',
    function() {
        
        /*
        Array.prototype.toJSON = function () {
            var str = "[";
            if (isNaN(this.length) || !isFinite(this.length)) return "{}";
            for (var i = 0; i < this.length; i++) {
                if (i > 0) str += ", ";
                str += this[i].toString();
            }
            str += "]";
            return str;
        }*/
        
        var uiState = {};
        var userData = {};
        var globalData = {};
        var avatar;
        var controller;
        var nextButtonId = 0;
        var _ui_will_close_callback = function() {};
        //var _user_data_changed_callback = function() {};
        var _global_data_did_change_callback = function() {};
        var _global_data_will_change_callback = function() {};
        var _keypress_callback = function () {};
        var _pending_buttons = [];
        var isOpen = false;
        var EZUI_ID_PREFIX = "myezui_";
        
        $("<div id='ezui_ui' title=''><div id='ezui_main'></div><div id='ezui_buttons'></div><div id='ezui_script_wrapper'></div></div>").appendTo('body');

        $( "#ezui_ui" ).dialog({
            width: 'auto',
            height: 'auto',
            modal: false,
            autoOpen: false,
            beforeClose: function(event, ui) { __handle_ui_will_close(); }
        });
        
        /* ==================== * ==========================================================================
         *  I/F with ezuiViewer *
         *  and private funcs   *
         * ==================== */
        
        _element_with_id_exists = function(id) {
            return $("#"+id).length > 0;
        };
        
        __convert_to_array = function(obj) {
            var arr = [];
            if (typeof(obj.length) === 'undefined') return undefined;
            for (var i = 0; i < obj.length; i++) {
                arr[i] = obj[i];
            }
            return arr;
        }
        
        __script = function(s) {
            $('#ezui_script_wrapper').html("<script type='text/javascript'>"+s+"</script>");
        };
        
        __set_user_data = function(obj) {
            userData = obj;
        };
        
        __set_global_data = function(key, value) {
            globalData[key] = value;
        };
        
        __handle_ui_will_close = function() {
            _ui_will_close_callback();
            isOpen = false;
            sirikata.event("sendMsgController", {disconnect: _('viewer').id});
        };
        
        /*
        __handle_user_data_changed = function(arr) {
            _user_data_will_change_callback();
            if (arr && arr.length) {
                for (i in arr) {
                    userData[arr[i].key] = arr[i].newval;
                }
            }
            _user_data_did_change_callback();
        }
        */
        
        __handle_global_data_changed = function(changes) {
            _global_data_will_change_callback();
            globalData = changes;
            //for (p in changes) {
            //    if (changes.hasOwnProperty(p) && globalData.hasOwnProperty(p)) {
            //        globalData[p] = changes[p];
            //    }
            //}
            _global_data_did_change_callback();
        };
        
        __init_viewer = function(px, py, pz, vx, vy, vz, ox, oy, oz, ow, ovx, ovy, ovz, ovw, mesh, id) {
            var pos = {x: px, y: py, z: pz};
            var vel = {x: vx, y: vy, z: vz};
            var ori = {x: ox, y: oy, z: oz, w: ow};
            var orivel = {x: ovx, y: ovy, z: ovz, w: ovw};
            _('viewer', {pos: pos, vel: vel, ori: ori, orivel: orivel, mesh: mesh, id: id});
        };
        
        __reset = function () {
            $('#ezui_main').html("");
            $('#ezui_script_wrapper').html("");
            uiState = {};
            userData = {};
            globalData = {};
            avatar = undefined;
            controller = undefined;
            nextButtonId = 0;
            _ui_will_close_callback = function() {};
            _user_data_changed_callback = function() {};
            _global_data_changed_callback = function() {};
            _pending_buttons = [];
        };
        
        __handle_keypress = function(evt) {
            var evtobj = window.event ? event : evt; //distinguish between IE's explicit event object (window.event) and Firefox's implicit.
            var unicode = evtobj.charCode ? evtobj.charCode : evtobj.keyCode;
            var actualkey = String.fromCharCode(unicode);
            _key_pressed_callback(actualkey.toLowerCase());
        };
        
        __setup_button_callback = function(button) {
            sirikata.ui.button('#'+button.id).click(
                function() {
                    button.cb($("#"+this.id).text());
                }
            );
        };
        
        __setup_buttons = function() {
            if (typeof(_("_written_buttons")) === 'undefined') _("_written_buttons", []);
            _("_written_buttons", __convert_to_array(_("_written_buttons")));
            for (i in _pending_buttons) {
                if (_element_with_id_exists(_pending_buttons[i].id)) {
                    __setup_button_callback(_pending_buttons[i]);
                    _("_written_buttons").push(_pending_buttons[i]);
                    _pending_buttons.splice(i, 1);
                }
            }
        };
        
        __set_controller = function(val) {
            controller = val;
        };
        
        __set_viewer = function(val) {
            avatar = val;
        };

        
        __move_to_picked_position = function(x, y) {
            h = $(window).height();
            w = $(window).width();
            x = (x+1)/2*w;
            y = (1-y)/2*h;
            moveTo(x, y);
        };
        
        
        /* ==================== * ==========================================================================
         *     DEVELOPER API    *
         * ==================== */
        
        write = function(html) {
            $('#ezui_main').append(html.toString());
            if (!isOpen) {
                isOpen = true;
                $('#ezui_ui').dialog('open');
            }
            __setup_buttons();
        };
        
        append = function(id, html) {
            $("#"+id).append(html.toString());
            __setup_buttons();
        };
        
        fill = function(id, html) {
            $("#"+id).html(html.toString());
            __setup_buttons();
        };
        
        setInnerHTML = function(id, html) {
            fill(id, html.toString());
        };
        
        getInnerHTML = function(id) {
            return $("#"+id).html();
        };
        
        clear = function (id) {
            if (!id) {
                id = "ezui_main";
                nextButtonId = 0;
            }
            $('#'+id).html("");
        };
        
        close = function () {
            __handle_ui_will_close(); 
            $("#ezui_ui").dialog('close');
        };
        
        restoreHTML = function() {
            if (_("_saved_html") && _("_next_button_id") != undefined) {
                clear();
                write(_("_saved_html"));
                nextButtonId = _("_next_button_id");
                if (_("_written_buttons")) {
                    _pending_buttons = _("_written_buttons");
                    _("_written_buttons", []);
                    __setup_buttons();
                }
                return $('#ezui_main').html();
            } else {
                return undefined;
            }
        };
        restoreHTMLOr = function(func) {
            if (_("_saved_html") && _("_next_button_id") != undefined) restoreHTML();
            else func();
        };
        
        save = function() {
            _("_saved_html", $('#ezui_main').html());
            _("_next_button_id", nextButtonId);
            var msg = { user: _('viewer').id, userData: userData };
            sendToController(msg);
        };
        
        moveTo = function(x, y) {
            $('#ezui_ui').dialog('option', 'position', [x, y]);
        };
        
        /* known bug: there is currently no checking of the passed-in id values
        * for validity.  If they contain quotes, the UI could break badly! */
        sections = function(/*...*/) {
            var s = "";
            for (var i = 0; i < arguments.length; i++) {
                if (arguments[i].toString) {
                    s += "<div id='"+arguments[i]+"'></div><hr />"
                }
            }
            return s;
        };
        
        Button = function(text, callback, id) {
            this.text = text;
            this.id = id;
            this.str = "<button id='"+this.id+"'>"+this.text+"</button>";
            this.toString = function() { return this.str; };
            this._en = true;
            this.cb = (typeof(callback) === 'function') ? callback : function() {};
            this.attrs = {};
            this.attrString = function () {
                var s = "";
                for (i in attrs) {
                    if (typeof(attrs[i]) !== 'undefined') s += (i + "='"+attrs[i]+"' ");
                }
                return s;
            }
            this.updateString = function() {
                this.str = "<button id='"+this.id+"' "+this.attrString()+">"+this.text+"</button>";
            }
            //this.cb = callback;
            this.enabled = function(val) {
                var old = this._en;
                if (typeof(val) !== 'undefined') {
                    if (val) {
                        this._en = true;
                        $("#"+this.id).removeAttr("disabled");
                        this.attrs["disabled"] = undefined;
                    } else {
                        this._en = false;
                        $("#"+this.id).attr("disabled", "disabled");
                        this.attrs["disabled"] = "disabled";
                    }
                }
                return old;
            }
        };
        
        button = function(text, callback, id) {
            if (typeof(id) === 'undefined') id = "ezui_button_"+(nextButtonId++);
            if (typeof(text) === 'undefined') text = "";
            b = new Button(text, callback, id.toString());
            _pending_buttons.push(b);
            return b;
        };
        
        label = function (id, text) {
            return "<span id='"+id+"'>" + ( (typeof(text) === 'undefined') ? "" : text ) + "</span>";
        };
        
        onUiWillClose = function(func) {
            _ui_will_close_callback = func;
        };
        
        /*onUserDataChanged = function(func) {
            _user_data_changed_callback = func;
        }*/
        
        onGlobalDataDidChange = function(func) {
            _global_data_did_change_callback = func;
        };
        
        onGlobalDataWillChange = function(func) {
            _global_data_will_change_callback = func;
        };
        
        onKeypress = function(func) {
            _keypress_callback = func;
        }
        
        _ = function(key, value) {
            var old;
            if (typeof(userData) === 'undefined') {
                userData = {};
            }
            if (typeof(userData[key]) !== 'undefined') {
                old = userData[key];
            }
            if (typeof(value) !== 'undefined') {
                userData[key] = value;
            }
            return old;
        };
        
        GLOBAL_ = function(p) {
            if (typeof(globalData) === 'undefined') {
                globalData = {};
            }
            return globalData[p];
        };
        
        __ = GLOBAL_;
        g_ = GLOBAL_;
        
        syncUserData = function() {
            var msg = { user: avatar, userData: userData, globalData: globalData };
            sendToController(msg);
        };
        
        sendToController = function(msg) {
            msg.ezui_event = 0;
            sirikata.event("sendMsgController", msg);
        };
        
        sendToViewer = function(msg) {
            msg.ezui_event = 0;
            sirikata.event("sendMsgViewer", msg);
        };
        
        send = function(msg, pres) {
            msg.ezui_event = 0;
            sirikata.event("sendMsg", {msg: msg, pres: pres});
        };
        
        notifyController = function(buttonText) {
            var av = _('viewer').id;
            sirikata.event("sendMsgController", { ezui_button_pressed: buttonText, avatar: av } );
        };
        
        notifyControllerAnd = function(cb) {
            return function (buttonText) {
		notifyController(buttonText);
		cb(buttonText);
            };
        };
        
        prettyprint = function(obj) {
            sirikata.event("pretty", {obj: obj});
        };
        
        /*
        sirikata.ui.window( 
            '#ezui', //#selector 
            { 
                beforeClose: function(event, ui) {
                    sirikata.log('fatal', '\n\n\n ui ='+ui+'\n\n\n');
                    if (ui == "ezui") return __handle_ui_will_close();
                    return true;
                }
            } 
        );
        */
        

    }
);