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
    'fl',
    function() {
        
        var uiState = {};
        var userData = {};
        var globalData = {};
        var avatar;
        var controller;
        var nextControlId = 0;
        /**
         * @ignore
         */
        var _ui_will_close_callback = function() {};
        /**
         * @ignore
         */
        var _global_data_did_change_callback = function() {};
        /**
         * @ignore
         */
        var _global_data_will_change_callback = function() {};
        /**
         * @ignore
         */
        var _keypress_callback = function () {};
        var _pending_buttons = [];
        var _iframe_count = 0;
        var isOpen = false;
        var FL_ID_PREFIX = "myflatland_";
        
        var sirikata_ui_location = window.location;
        
        $("<div id='fl_ui' title=''><div id='fl_main'></div><div id='fl_buttons'></div><div id='fl_script_wrapper'></div></div>").appendTo('body');

        $( "#fl_ui" ).dialog({
            width: 'auto',
            height: 'auto',
            modal: false,
            autoOpen: false,
            beforeClose: function(event, ui) { __handle_ui_will_close(); }
        });
        
        /* ==================== * ==========================================================================
         *  I/F with flatlandViewer *
         *  and private funcs   *
         * ==================== */
        
        isUndefined = function(a) {
            return (typeof(a) === 'undefined');
        }
        /**
         * @ignore
         */
        _element_with_id_exists = function(id) {
            return $("#"+id).length > 0;
        };
        
        /**
         * @ignore
         */
        __convert_to_array = function(obj) {
            var arr = [];
            if (typeof(obj.length) === 'undefined') return undefined;
            for (var i = 0; i < obj.length; i++) {
                arr[i] = obj[i];
            }
            return arr;
        }
        
        /**
         * @ignore
         */
        __script = function(s) {
            $('#fl_script_wrapper').html("<script type='text/javascript'>"+s+"</script>");
        };
        
        /**
         * @ignore
         */
        __set_user_data = function(obj) {
            userData = obj;
        };
        
        /**
         * @ignore
         */
        __set_global_data = function(key, value) {
            globalData[key] = value;
        };
        
        /**
         * @ignore
         */
        __handle_ui_will_close = function() {
            _ui_will_close_callback();
            isOpen = false;
            sirikata.event("sendMsgController", {disconnect: _('viewer').id});
        };
        
        /**
         * @ignore
         */        
        __handle_global_data_changed = function(changes) {
            _global_data_will_change_callback();
            globalData = changes;
            _global_data_did_change_callback();
        };
        
        /**
         * @ignore
         */
        __init_viewer = function(px, py, pz, vx, vy, vz, ox, oy, oz, ow, ovx, ovy, ovz, ovw, mesh, id) {
            var pos = {x: px, y: py, z: pz};
            var vel = {x: vx, y: vy, z: vz};
            var ori = {x: ox, y: oy, z: oz, w: ow};
            var orivel = {x: ovx, y: ovy, z: ovz, w: ovw};
            _('viewer', {pos: pos, vel: vel, ori: ori, orivel: orivel, mesh: mesh, id: id});
        };
        
        /**
         * @ignore
         */
        __reset = function () {
            $('#fl_main').html("");
            $('#fl_script_wrapper').html("");
            uiState = {};
            userData = {};
            globalData = {};
            avatar = undefined;
            controller = undefined;
            nextControlId = 0;
            _ui_will_close_callback = function() {};
            _user_data_changed_callback = function() {};
            _global_data_changed_callback = function() {};
            _pending_buttons = [];
        };
        
        /**
         * @ignore
         */
        __handle_keypress = function(evt) {
            var evtobj = window.event ? event : evt; //distinguish between IE's explicit event object (window.event) and Firefox's implicit.
            var unicode = evtobj.charCode ? evtobj.charCode : evtobj.keyCode;
            var actualkey = String.fromCharCode(unicode);
            _key_pressed_callback(actualkey.toLowerCase());
        };
        
        /**
         * @ignore
         */
        __setup_button_callback = function(button) {
            sirikata.ui.button('#'+button.id).click(
                function() {
                    button.cb($("#"+this.id).html(), button.data);
                }
            );
        };
        
        /**
         * @ignore
         */
        __setup_buttons = function() {
            if (typeof(_("_written_buttons")) === 'undefined') _("_written_buttons", []);
            _("_written_buttons", __convert_to_array(_("_written_buttons")));
            for (i = 0; i < _pending_buttons.length; i++) {
                if (_element_with_id_exists(_pending_buttons[i].id)) {
                    if (_pending_buttons[i].type == "button") __setup_button_callback(_pending_buttons[i]);
                    _("_written_buttons").push(_pending_buttons[i]);
                    _pending_buttons.splice(i, 1);
                    i--;
                }
            }   
        };
        
        /**
         * @ignore
         */
        __set_controller = function(val) {
            controller = val;
        };
        
        /**
         * @ignore
         */
        __set_viewer = function(val) {
            avatar = val;
        };

        /**
         * @ignore
         */
        __move_to_picked_position = function(x, y) {
            h = $(window).height();
            w = $(window).width();
            x = (x+1)/2*w;
            y = (1-y)/2*h;
            moveTo(x, y);
        };
        
        /**
         * @ignore
         */
        __attr_string = function(obj) {
            if (isUndefined(obj)) return "";
            var ret = "";
            for (a in obj) {
                if (obj.hasOwnProperty(a)) {
                    if (typeof(obj[a]) === 'string') {
                        ret += a + "='" + obj[a] + "' ";
                    } else if (typeof(obj[a]) === 'object') {
                        // attribute is style
                        ret += a + "='" + (function(o) {
                            var ret = ""
                            for (i in o) {
                                if (o.hasOwnProperty(i)) {
                                    ret += i + ":" + o[i] + ";";
                                }
                            }
                            return ret;
                        })(obj[a]) + "' ";
                    }
                }
            }
            return ret;
        }
        
        /* ==================== * ==========================================================================
         *     DEVELOPER API    *
         * ==================== */
        
        /**
         * @param a
         * @returns true if the parameter is undefined; else false
         */
        isUndefined = function(a) {
            return (typeof(a) === 'undefined');
        }
        
        /**
         * Appends the given HTML to the end of the UI.
         * @param {String} html The HTML to write.
         */
        write = function(html) {
            $('#fl_main').append(html.toString());
            if (!isOpen) {
                isOpen = true;
                $('#fl_ui').dialog('open');
            }
            __setup_buttons();
        };
        
        /**
         * Appends the given HTML to the end of the element with the given id.
         * @param {String} id The id of the element to append to.
         * @param {String} html The HTML to append.
         */
        append = function(id, html) {
            $("#"+id).append(html.toString());
            __setup_buttons();
        };
        
        /**
         * Sets the content of the element with the given id to the given html.  Same functionality as setInnerHTML().
         * @see setInnerHTML
         * @param {String} id The id of the element whose content you want to change.
         * @param {String} html The HTML to set as the element's content.
         * @example
         * write(p("initial text", attributes("id", "myParagraph")));
         * fill("myParagraph", "new text");
         */
        fill = function(id, html) {
            $("#"+id).html(html.toString());
            __setup_buttons();
        };
        
        /**
         * Sets the content of the element with the given id to the given html.  Same functionality as fill().
         * @see fill
         * @param {String} id The id of the element whose content you want to change.
         * @param {String} html The HTML to set as the element's content.
         * @example
         * write(p("initial text", attributes("id", "myParagraph")));
         * setInnerHTML("myParagraph", "new text");
         */
        setInnerHTML = function(id, html) {
            fill(id, html.toString());
        };
        
        /**
         * Gets the content of the element with the given id.
         * @param {String} id The id of the element whose content you want to get.
         * @returns {String} The html inside the element with the specified id.
         */
        getInnerHTML = function(id) {
            return $("#"+id).html();
        };
        
        /**
         * Clears the contents of the element with the specified id (i.e. sets its innerHTML to the empty string)
         * @param {String} id The id of the element to clear.
         */
        clear = function (id) {
            if (!id) {
                id = "fl_main";
                nextControlId = 0;
            }
            $('#'+id).html("");
        };
        
        /**
         * Closes the UI window.
         */
        close = function () {
            __handle_ui_will_close(); 
            $("#fl_ui").dialog('close');
        };
        
        /**
         * @deprecated since 8/21/2011; restoring HTML may cause some callbacks to break and user data to be left in an inconsistent state.
         * It is strongly recommended that you construct the UI from scratch each time it is loaded.
         * @returns {String} The restored html, or undefined if no html was saved.
         */
        restoreHTML = function() {
            if (_("_saved_html") && _("_next_button_id") != undefined) {
                clear();
                write(_("_saved_html"));
                nextControlId = _("_next_button_id");
                if (_("_written_buttons")) {
                    _pending_buttons = _("_written_buttons");
                    _("_written_buttons", []);
                    __setup_buttons();
                }
                return $('#fl_main').html();
            } else {
                return undefined;
            }
        };
        
        /**
         * @deprecated since 8/21/2011; restoring HTML may cause some callbacks to break and user data to be left in an inconsistent state.
         * It is strongly recommended that you construct the UI from scratch each time it is loaded.
         * @param {Function} func
         */
        restoreHTMLOr = function(func) {
            if (_("_saved_html") && _("_next_button_id") != undefined) restoreHTML();
            else func();
        };
        
        /**
         * Saves user data to the controller, allowing it to persist between invocations of the UI.
         */
        save = function() {
            _("_saved_html", $('#fl_main').html());
            _("_next_button_id", nextControlId);
            var msg = { user: _('viewer').id, userData: userData };
            sendToController(msg);
        };
        
        /**
         * Sets the position of the UI window on screen to the given point, measured in pixels from the top-left corner of the client window.
         * @param {Number} x
         * @param {Number} y
         */
        moveTo = function(x, y) {
            $('#fl_ui').dialog('option', 'position', [x, y]);
        };
        
        /**
         * Sets the size of the UI window to the given dimensions, in pixels.
         * @param {Number} width
         * @param {Number} height
         */
        resize = function(width, height) {
            $('#fl_ui').dialog('option', 'size', [width, height]);
        }
        
        /* known bug: there is currently no checking of the passed-in id values
        * for validity.  If they contain quotes, the UI could break badly! */
        
        /**
         * Creates a series of formatted &lt;div&gt;s with the given ids.
         * @param {...String} arguments The ids of the sections to create
         * @returns {String} the created sections (divs) as html
         */
        sections = function(/*...*/) {
            var s = "";
            for (var i = 0; i < arguments.length; i++) {
                if (arguments[i].toString) {
                    s += "<div id='"+arguments[i]+"'></div><hr />"
                }
            }
            return s;
        };
        
        /*
        webpage = function(url, width, height) {
            _iframe_count++;
            var iframeid = "fl_iframe_" + _iframe_count;
            if (width <= 0 || typeof(width) !== "number") width = 300;
            if (height <= 0 || typeof(height) !== "number") height = 200;
            if (url.substring(0, "http://".length) != "http://") url = "http://"+url;
            $.get(url, {}, function (s) {
                write(s);
                //$('#'+iframeid).html(s);
            });
            document.onbeforeunload = function (e) {
                e = e || window.event;
                s="An external webpage is trying to redirect you.  This will prevent you from interacting with Sirikata until you close and restart the application.\
                  It is strongly recommended that you click 'cancel' below to prevent the redirect.";
                e.returnValue = s;
                return s;
            };
            return "<iframe id='"+iframeid+"' width='"+width+"' height='"+height+"' frameborder='1'></iframe>";
        }*/
        
        /**
         * @class
         */
        Textbox = function(size, id) {
            this.type = "textbox";
            this._attrs = {id: id, size: size};
            this.toString = function () { return "<input type='text' "+this.attrString()+"></input>"; };
            /**
             * @ignore
             */
            this.updateAttributes = function() {
                for (a in this._attrs) {
                    $('#'+this._attrs.id).attr(a, this._attrs[a]);
                }
            }
            /**
             * @ignore
             */
            this.attrString = function () {
                var s = "";
                for (i in this._attrs) {
                    if (typeof(this._attrs[i]) !== 'undefined') s += (i + "='"+this._attrs[i]+"' ");
                }
                return s;
            }
            /**
             * @ignore
             */
            this._getOrSetAttr = function(key, val) {
                this._attrs[key] = $('#'+this._attrs[key]).attr();
                var old = this._attrs[key];
                if (typeof(val) !== 'undefined') {
                    this._attrs[key] = val;
                }
                return old;
            }
            /**
             * Gets the user-entered text inside this textbox.  If the parameter is defined, sets the text inside the textbox.
             * @param {String} val The text to set as the value of this Textbox.
             * @returns {String} val The content of this Textbox before the new value (if any) is assigned.
             */
            this.value = function (val) { return this._getOrSetAttr("value", val) };
        }
        
        /**
         * Creates a textual user input object with the given size and id.  The object returned can be passed
         * to write(), append(), fill(), or setInnerHTML() to draw the textbox on the screen.
         * @param {Number} size The size attribute of the &lt;input&gt; element created.
         * @param {String} id The id of the element created.
         * @returns {Object} the textbox object.
         */
        textbox = function(size, id) {
            if (typeof(id) === 'undefined') id = "fl_control_"+(nextControlId++);
            if (size <= 0 || typeof(size) != 'number') size = 16;
            t = new Textbox(size, id.toString());
            _pending_buttons.push(t);
            return t;
        }
        
        /**
         * @ignore
         */
        Image = function(src, h, w, alt, id) {
            alt = alt || "";
            src = src || "";
            h = h || 1;
            w = w || 1;
            this._attrs = {src: src, alt: alt, id: id}
            if (h && h > 0) this._attrs.height = h;
            if (w && w > 0) this._attrs.width = w;
            this.toString = function() {
                return "<img "+this.attrString()+"/>";
            }
            this.updateAttributes = function() {
                for (a in this._attrs) {
                    $('#'+this._attrs.id).attr(a, this._attrs[a]);
                }
            }
            this.attrString = function () {
                var s = "";
                for (i in this._attrs) {
                    if (typeof(this._attrs[i]) !== 'undefined') s += (i + "='"+this._attrs[i]+"' ");
                }
                return s;
            }
            this._attr_meta = function(attr) {
                return function (val) {
                    var ret = this;
                    if (!isUndefined(val)) {
                        ret = this._attrs[attr]
                    }
                }
            }
            this.src = function(newsrc) {
                if (!isUndefined(newsrc)) {
                    this._attrs.src = newsrc
                    this.updateAttributes();
                    return this;
                } else {
                    return this._attrs.src;
                }
            }
        }
        
        /**
         * Creates an image with the given source URL, dimensions, and alt-text.
         * @param {String} src The URL of the image.
         * @param {Number} h The height of the image element onscreen, in pixels.
         * @param {Number} w The width of the image element onscreen, in pixels.
         * @param {String} alt The alt-text to use for the image.
         * @returns {Object} the image object
         */
        image = function(src, h, w, alt) {
            return new Image(src, h, w, alt); //"<img src='"+src+"' alt='"+alt+"' />";
        }
        
        /**
         * @class
         */
        Button = function(text, callback, data, id) {
            this.type = "button";
            this.id = id
            this.text = text;
            this.data = data;
            this.toString = function() { return "<button "+this.attrString()+">"+this.text+"</button>"; };
            this._en = true;
            this.cb = (typeof(callback) === 'function') ? callback : function() {};
            this._attrs = {id: id, type: "button"};
            this.updateAttributes = function() {
                for (a in this._attrs) {
                    $('#'+this._attrs.id).attr(a, this._attrs[a]);
                }
            }
            this.attrString = function () {
                var s = "";
                for (i in this._attrs) {
                    if (typeof(this._attrs[i]) !== 'undefined') s += (i + "='"+this._attrs[i]+"' ");
                }
                return s;
            }
            /**
             * Returns true if this Button is enabled, else false.  If the parameter is defined, enables or disables the button (works whether or not the button has already been written to the UI).
             * @param {Boolean} val If true, the button will be enabled
             * @returns {Boolean} the enabled state of this Button.
             */
            this.enabled = function(val) {
                var old = this._en;
                if (typeof(val) !== 'undefined') {
                    if (val) {
                        this._en = true;
                        $("#"+this._attrs.id).removeAttr("disabled");
                        this._attrs["disabled"] = undefined;
                    } else {
                        this._en = false;
                        $("#"+this._attrs.id).attr("disabled", "disabled");
                        this._attrs["disabled"] = "disabled";
                    }
                }
                return old;
            }
        };
        
        /**
         * Creates a button with the given display text, callback, data to pass to the callback, and ID.
         * @param {String} text Text to be displayed on the button.
         * @param {Function} callback Function to be invoked when the button is clicked.
         * @param data {Any} Object to be passed as the second parameter of the button callback.
         * @param {String} [optional] id
         * @returns {Object} button object
         */
        button = function(text, callback, data, id) {
            if (typeof(id) === 'undefined') id = "fl_control_"+(nextControlId++);
            if (typeof(text) === 'undefined') text = "";
            b = new Button(text, callback, data, id.toString());
            _pending_buttons.push(b);
            return b;
        };
        
        /**
         * Wraps the specified text in a <span> tag.
         * @param {String} id
         * @param {String} [optional] text
         * @returns {String} An inline HTML element (span) with the specified id
         */
        label = function (id, text) {
            return "<span id='"+id+"'>" + ( (typeof(text) === 'undefined') ? "" : text ) + "</span>";
        };
        
        /**
         * @class
         */
        HtmlTag = function (tag, innerHTML, attrArray) {
            this._attrs = { id: "fl_control_" + (nextControlId++) };
            this.tag = tag;
            this.html = innerHTML.join("");
            this.createAttributes = function (attrlist) {
                for (i in attrlist) {
                    var a = attrlist[i];
                    if (isUndefined(this._attrs[a])) this._attrs[a] = "";
                    this[a] = function (val) {
                        var attr = arguments.callee.attr;
                        if (!isUndefined(val)) {
                            var id = this._attrs.id;
                            this._attrs[attr] = val.toString();
                            this.updateDisplay(id);
                            return this;
                        } else {
                            return this._attrs[attr];
                        }
                    };
                    this[a].attr = a;
                }
            }
            this.createAttributes(attrArray);
            this.updateDisplay = function (old_id) {
                for (a in this._attrs) {
                    if (a != "id" && this._attrs.hasOwnProperty(a)) {
                        $('#'+old_id).attr(a, this._attrs[a]);
                    }
                }
                $('#'+old_id).html(this.html);
                if (!isUndefined(this._attrs.id)) $('#'+old_id).attr("id", this._attrs.id);
            }
            this.toString = function () {
                return "<"+this.tag+" "+__attr_string(this._attrs)+">"+this.html+"</"+this.tag+">";
            }
            /**
             * Sets the attributes of this HtmlTag.  Updates the UI to reflect the changes.
             * @param {Object} a The attributes object to assign.
             * @example
             * var myParagraph = p("grumpy wizards make toxic brew for the evil queen and Jack");
             * var pStyle = style("color", "#888", "text-decoration", "none");
             * p.set(attributes("id", "my_id", "style", pStyle));
             */
            this.set = function(a) {
                var id = isUndefined(this._attrs) ? undefined : this._attrs.id;
                this._attrs = a;
                if (isUndefined(this._attrs)) this._attrs = {};
                if (isUndefined(this._attrs.id)) this._attrs.id = id;
                this.updateDisplay(id);
            }
            /**
             * Sets a single attribute of this HtmlTag.  Updates the UI to reflect the change.
             * @param {String} key The name of the attribute to change.
             * @param {String} value The value to assign to the attribute.
             * @deprecated You should use the xetters for the individual attributes (e.g. style(), class()) instead.
             */
            this.setAttribute = function (key, value) {
                if (isUndefined(this._attrs)) {
                    this._attrs = {};
                }
                var id = this._attrs.id;
                this._attrs[key] = value;
                if (isUndefined(this._attrs.id)) this._attrs.id = id;
                this.updateDisplay(id);
            }
            
            this.innerHTML = function () {
                var html = ""
                for (var i = 0; i < arguments.length; i++) {
                    var a = arguments[i];
                    if (!isUndefined(a.toString)) html += a;
                }
                if (arguments.length == 0) html = undefined;
                if (!isUndefined(html)) {
                    this.html = html;
                    $('#'+this._attrs.id).html(html);
                    return this;
                } else {
                    return this.html;
                }
            }
        };
        
        /**
         * Creates an object that represents an HTML element with the specified tag, content, and attributes.
         * @param {String} tag The html tag to create
         * @param {String|Object} innerHTML The html to place inside this tag (can be a string or another tag object);
         * @param {Object} attrs An attribute object to apply to the tag.
         */
        htmlTag = function (tag, innerHTMLArray, attrArray) {
            if (isUndefined(attrArray)) attrArray = [];
            var newAttrArray = attrArray.concat(["id", "style", "class", "title"]);
            var t = new HtmlTag(tag, innerHTMLArray, newAttrArray);
            return t;
        };
        
        /**
         * Creates an object representing a set of attribute-value pairs that can be passed to HtmlTag's attrs() method.
         * @returns {Object} An attribute object.
         */
        attributes = function () {
            var ret = {};
            for (var i = 0; i < arguments.length; i+=2) {
                ret[arguments[i]] = arguments[i+1];
            }
            return ret;
        }
        
        /**
         * @ignore
         */
        __tag_metaconstructor = function(tag, attrArray) {
            return function () { return htmlTag(tag, Array.prototype.slice.call(arguments), attrArray); };
        }
        
        
        
        var evts = ["onclick", "ondblclick", "onmousedown", "onmousemove",
                    "onmouseout", "onmouseover", "onmouseup", "onkeydown",
                    "onkeypress", "onkeyup"];
        var lang = ["dir", "lang", "xml:lang"];
        
        address =    __tag_metaconstructor("address", lang.concat(evts));
        area =       __tag_metaconstructor("area", ["coords", "href", "nohref", "shape", "target", "accesskey", "tabindex"].concat(lang).concat(evts));
        blockquote = __tag_metaconstructor("blockquote", lang.concat(evts));
        bold =       __tag_metaconstructor("b", lang.concat(evts));
        caption =    __tag_metaconstructor("caption", lang.concat(evts));
        cite =       __tag_metaconstructor("cite", lang.concat(evts));
        code =       __tag_metaconstructor("code", lang.concat(evts));
        col =        __tag_metaconstructor("col", ["align", "char", "charoff", "span", "valign", "width"].concat(lang).concat(evts));
        colgroup =   __tag_metaconstructor("colgroup", ["align", "char", "charoff", "span", "valign", "width"].concat(lang).concat(evts));
        del =        __tag_metaconstructor("del", ["cite", "datetime"].concat(lang).concat(evts));
        dfn =        __tag_metaconstructor("dfn", lang.concat(evts));
        dl =         __tag_metaconstructor("dl", evts);
        dd =         __tag_metaconstructor("dd", lang.concat(evts));
        div =        __tag_metaconstructor("div", lang.concat(evts));
        dt =         __tag_metaconstructor("dt", evts);
        em =         __tag_metaconstructor("em", lang.concat(evts));
        fieldset =   __tag_metaconstructor("fieldset", lang.concat(evts));
        h1 =         __tag_metaconstructor("h1", lang.concat(evts));
        h2 =         __tag_metaconstructor("h2", lang.concat(evts));
        h3 =         __tag_metaconstructor("h3", lang.concat(evts));    
        h4 =         __tag_metaconstructor("h4", lang.concat(evts));
        h5 =         __tag_metaconstructor("h5", lang.concat(evts));
        h6 =         __tag_metaconstructor("h6", lang.concat(evts));
        hr =         __tag_metaconstructor("hr", lang.concat(evts));
        italic =     __tag_metaconstructor("i", lang.concat(evts));
        img =        __tag_metaconstructor("img", ["src", "alt", "width", "height", "border", "hspace", "vspace", "longdesc", "usemap"].concat(lang).concat(evts));
        ins =        __tag_metaconstructor("ins", ["cite", "datetime"].concat(lang).concat(evts));
        kbd =        __tag_metaconstructor("kbd", lang.concat(evts));
        label =      __tag_metaconstructor("label", ["for"].concat(lang).concat(evts));
        legend =     __tag_metaconstructor("legend", lang.concat(evts));
        li =         __tag_metaconstructor("li", evts.concat(lang));
        map =        __tag_metaconstructor("map", ["name"].concat(lang).concat(evts));
        object =     __tag_metaconstructor("object", ["align", "archive", "border", "classid", "codebase",
                                                      "codetype", "data", "declare", "height", "hspace",
                                                      "name", "standby", "type","usemap", "vspace",
                                                      "width"].concat(evts).concat(lang));
        ol =         __tag_metaconstructor("ol", ["type", "start"].concat(evts).concat(lang));
        optgroup =   __tag_metaconstructor("optgroup", ["label", "disabled"].concat(evts).concat(lang));
        option =     __tag_metaconstructor("option", ["disabled", "label", "selected", "value"].concat(evts).concat(lang));
        pre =        __tag_metaconstructor("pre", evts.concat(lang));
        paragraph =  __tag_metaconstructor("p", evts.concat(lang));
        quote =      __tag_metaconstructor("q", ["cite"].concat(evts).concat(lang));
        samp =       __tag_metaconstructor("samp", lang.concat(evts));
        select =     __tag_metaconstructor("select", ["disabled", "multiple", "name", "size", "onblur"].concat(evts).concat(lang));
        span =       __tag_metaconstructor("span", evts.concat(lang));
        strong =     __tag_metaconstructor("strong", lang.concat(evts));
        sub =        __tag_metaconstructor("sub", evts.concat(lang));
        sup =        __tag_metaconstructor("sup", evts.concat(lang));
        table =      __tag_metaconstructor("table", ["border", "align", "width", "cellpadding", "cellspacing", "rules", "summary"]);
        tr =         __tag_metaconstructor("tr" ["align", "valign", "height"]);
        row = function() {
            var cells = "";
            for (var i = 0; i < arguments.length; i++) {
                cells += td(arguments[i]);
            }
            return tr(cells);
        }
        td =         __tag_metaconstructor("td", ["align", "valign", "colspan", "width"]);
        textarea =   __tag_metaconstructor("textarea", ["accesskey", "cols", "disabled", "name", "readonly", "rows", "onblur"].concat(evts).concat(lang));
        ul =         __tag_metaconstructor("ul", evts.concat(lang));
        vartext =    __tag_metaconstructor("var", lang.concat(evts));
        
        /**
         * @class
         */
        Style = function(obj) {
            this.o = obj;
            /**
             * Converts this Style instance to a CSS-formatted string, suitable for use in the style attribute of an HTML element.
             * @returns {String} the formatted style string.
             */
            this.toString = function() {
                if (!isUndefined(this.s)) return this.s;
                var ret = ""
                for (a in this.o) {
                    if (this.o.hasOwnProperty(a)) {
                        ret += a + ":" + this.o[a] + ";";
                    }
                }
                this.s = ret;
                return ret;
            }
        }
        
        /**
         * Creates an object representing a CSS style which can be applied as the value of the style attribute when creating an attributes object.
         * @see attributes
         * @param {...} arguments Attribute/Value pairs
         * @returns {Object} an object representing the style
         * @example
         * var myStyle = style("font-family", "Helvetica", "color", "#aa0000");
         * write(p("red text in a paragraph", attributes("style", myStyle)));
         */
        style = function() {
            var o = {}
            for (var i = 0; i < arguments.length; i+=2) {
                o[arguments[i]] = arguments[i+1];
            }
            return new Style(o);
        }
        
        
        /**
         * Sets a callback to be executed when the UI is closed.
         * @param {Function} callback The callback to be invoked when the UI window is closed (either by the user or some other process).
         */
        onUiWillClose = function(callback) {
            _ui_will_close_callback = callback;
        };
        
        /**
         * Sets a callback to be executed when the global data is changed by this UI's controller.
         * @param {Function} callback The callback to be invoked after global data has been changed.
         */ 
        onGlobalDataDidChange = function(callback) {
            _global_data_did_change_callback = callback;
        };
        
        /**
         * Sets a callback to be executed just before global data is changed.  This is useful if you want to do something with the old values.
         * @param {Function} callback The callback to be invoked before global data is changed.
         */ 
        onGlobalDataWillChange = function(callback) {
            _global_data_will_change_callback = callback;
        };
        
        /**
         * @ignore
         */
        onKeypress = function(callback) {
            _keypress_callback = callback;
        }
        
        /**
         * Accesses the user data variable with the given key, and sets its value if the second parameter is defined.
         * @param {String} key
         * @param value
         * @returns The value of the user data field with the specified name.  If the value is changed by this call, the old value is returned.
         */
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
        
        /**
         * Accesses the global data variable with the given key.
         * @param {String} key
         * @returns The value of the global data field with the specified name.
         */
        GLOBAL_ = function(p) {
            if (typeof(globalData) === 'undefined') {
                globalData = {};
            }
            return globalData[p];
        };
        
        __ = GLOBAL_;
        g_ = GLOBAL_;
        
        /**
         * Sends the given message to the UI's controller presence.
         * @param {Object} msg The message to send.
         */
        sendToController = function(msg) {
            msg.fl_event = 0;
            sirikata.event("sendMsgController", msg);
        };
        
        /**
         * Sends the given message to the avatar viewing the UI.
         * @param {Object} msg The message to send.
         */
        sendToViewer = function(msg) {
            msg.fl_event = 0;
            sirikata.event("sendMsgViewer", msg);
        };
        
        /**
         * Sends the given message to the given presence.
         * @param {Object} msg
         * @param {String} pres
         */
        send = function(msg, pres) {
            msg.fl_event = 0;
            sirikata.event("sendMsg", {msg: msg, pres: pres});
        };
        
        /**
         * Notifies the controller of a button event.  Set this function as a button's callback to automatically notify the controller when the button is pressed.
         * You should not call this function in your code.
         */
        notifyController = function(buttonText, data) {
            var av = _('viewer').id;
            sirikata.event("sendMsgController", { fl_button_pressed: buttonText, avatar: av, data: data } );
        };
        
        /**
         * Returns a callback that notifies the controller of a button event, in addition to calling the given function.  The callback returned can be set as a button callback.
         * @param {Function} func The function to call after notifying the controller.
         * @returns A function that notifies the controller of a button event and then calls the specified function with the button text as a parameter.
         */
        notifyControllerAnd = function(func) {
            return function (buttonText) {
		notifyController(buttonText);
		func(buttonText);
            };
        };
        
        /**
         * Prints the given object to the console, formatted with std.core.serialize().
         * @param {Object} obj
         */
        prettyprint = function(obj) {
            sirikata.event("pretty", {obj: obj});
        };
        
        repeatingTimer = function(period, cb) {
            millis = period*1000;
            return setTimeout(
                function () {
                    setTimeout(cb, millis);
                    cb();
                }, millis
            );
        }
        
    }
);