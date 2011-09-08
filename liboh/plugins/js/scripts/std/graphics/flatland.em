var _fl_global = {};

/* _fl_users maps USERS' PRESENCE IDS to HASHES OF VARNAMES -> DATA. */
var _fl_users = {};
var _fl_scripts = {}; /* maps presence ids to scripts*/
var _fl_curr_viewers = {}; /*hash of avatar visible IDs viewing this UI -> value true if currently viewing*/
var _fl_on_connection_callback = {};
var _fl_on_user_data_did_update_callback = {};
var _fl_on_user_data_will_update_callback = {};
var _fl_button_callback = {};
var _fl_gdata_update_timer;
var _fl_rickroll = @write('<iframe width="425" height="349" src="http://www.youtube.com/embed/oHg5SJYRHA0" frameborder="0" allowfullscreen></iframe>')@;

/** @namespace */
var fl = {};


/**
 * Sets the UI script for the given presence (defaults to system.self).  The UI script is a string of JavaScript
 * code that will be executed in a dialog window when a user right-clicks on the given presence.
 * @param {String} s The script to set as the UI script for the presence.
 * @param {Presence} me The presence to set the UI for.  Defaults to system.self.
 * @param {Boolean} clearData If true, all user and global data, including the IDs of avatars who have interacted with this UI, will be deleted.
 */
fl.script = function (s, me, clearData) {
    me = (me || system.self).toString();
    if (clearData) {
        _fl_users = {};
        _fl_global = {};
    }
    _fl_scripts[me] = s;
}

/**
 * Executes the given JavaScript in the UI.  To maintain modularity of the UI from the server, this should only be used for debugging.
 * @param {String} s Script to execute in the UI.
 */
fl.exec = function(s) {
    fl.sendAllViewers({uieval: s});
}

/**
 * Sends the given message to all avatars currently viewing any of the UIs defined for presences on this entity.
 * @param {Object} msg The message to send.
 */
fl.sendAllViewers = function (msg) {
    for (v in _fl_curr_viewers) {
        var isViewer = _fl_curr_viewers[v];
        if (isViewer) {
            msg >> system.createVisible(v) >> [];
        }
    } 
}

/**
 * Sets the callback to be invoked when a button on the UI is pressed (the button's callback must be set to notifyController).  The callback takes as parameters the text on the button and the button's scripter-defined metadata.
 * @param {Function} func The callback to execute.
 * @param {Presence} me The presence whose UI should trigger this callback.  Defaults to system.self.
 */
fl.onButtonPressed = function(func, me) {
    if (typeof(func) === 'function') {
        me = (me || system.self).toString();
        _fl_button_callback[me] = func;
    }
}

/**
 * Sets the callback to be invoked when the UI for the given presence is served to an avatar.  The callback takes as a parameter the ID of the avatar who requested the UI.
 * @param {Function} func The callback to execute.
 * @param {Presence} me The presence whose UI should trigger this callback.  Defaults to system.self.
 */
fl.onConnection = function(func, me) {
    me = (me || system.self).toString();
    _fl_on_connection_callback[me] = func;
}

/**
 * Sets the callback to be invoked after the UI for the given presence saves a user's data to the server.  The callback takes no parameters.
 * @param {Function} func The function to execute.
 * @param {Presence} me The presence whose UI should trigger this callback.  Defaults to system.self.
 */
fl.onUserDataDidChange = function (func, me) {
    me = (me || system.self).toString();
    _fl_on_user_data_did_update_callback[me] = func;
}

/**
 * Sets the callback to be invoked just before the UI for the given presence saves a user's data to the server.  The callback takes as a parameter the new user data.
 * If the callback returns false, the changes to user data will be rejected.  
 * @param {Function} func The function to execute.
 * @param {Presence} me The presence whose UI should trigger this callback.  Defaults to system.self.
 */
fl.onUserDataWillChange = function (func, me) {
    me = (me || system.self).toString();
    _fl_on_user_data_will_update_callback[me] = func;
}

/**
 * Gets the value of the user data variable with the given key (name), for the given user.
 * @param {Presence} user The ID of the avatar whose user data is to be retrieved.
 * @param {String} key The name of the variable to read.
 * @returns The value of the user variable.
 */
fl.userData = function (user, key) {
    user = user.toString();
    if (typeof(_fl_users) === 'undefined') _fl_users = {};
    if (typeof(_fl_users[user]) === 'undefined') _fl_users[user] = {};
    return _fl_users[user][key];
}

/**
 * For each user X, calls the given function passing X's user data as the parameter.
 * @param {Function} func The function to call.
 * @example
 * //This example finds the high score for a game by iterating over each user's score.
 *   var highscore = -1;
 *   var highscore_user = "";
 *   fl.iterateUserData(function(user) {
 *       if (user['score'] > highscore) {
 *           highscore = user['score'];
 *           highscore_user = user['viewer']['id'];
 *       }
 *   });
 *   system.print(highscore_user+" has the highest score of "+highscore+" points!\n");
 */
fl.iterateUserData = function (func) {
    for (u in _fl_users) {
        if (_fl_users.hasOwnProperty(u)) {
            func(_fl_users[u]);
        }
    }
}

/**
 * Gets the global data variable with the given key (name).
 * @param {String} key The name of the variable to read.
 * @returns The value of the global variable.
 */
fl.globalData = function (key) {
    if (typeof(_fl_global) === 'undefined') _fl_global = {};
    return _fl_global[key];
}

/**
 * Sets the global data variable with the given key (name) to the given value.
 * @param {String} key The name of the variable to write.
 * @param value The value to assign to the variable.
 */
fl.setGlobalData = function (key, value) {
    if (typeof(_fl_global) === 'undefined') {
        _fl_global = {};
    }
    _fl_global[key] = value;
    if (_fl_gdata_update_timer) {
        _fl_gdata_update_timer.suspend();
    }
    _fl_gdata_update_timer = system.timeout(0.1, fl_sendGdataUpdateMsg);
}

/**
 * @ignore
 */
fl_sendGdataUpdateMsg = function () {
    var msg = {};
    msg.updatedGlobalData = _fl_global;
    fl.sendAllViewers(msg);
}

/**
 * @ignore
 */
fl_respondToRequest = function (req, sender) {
    var resp = {};
    if (sender.toString() == req.sender) {
        //valid request
        resp.self = system.self;
        resp.script = _fl_scripts[req.recipient];
        
        resp.userData = _fl_users[req.sender];
        if (typeof(resp.userData) === 'undefined') resp.userData = {};
        
        resp.globalData = _fl_global;
        if (typeof(resp.globalData) === 'undefined') resp.globalData = {};
        
        _fl_curr_viewers[req.sender] = true;
        if (typeof(_fl_on_connection_callback[req.recipient]) === 'function') {
            _fl_on_connection_callback[req.recipient](req.sender);
        }
        req.makeReply(resp) >> [];
    } else {
        //user is trying to be funny.  Rickroll them.
        resp.script = _fl_rickroll;
        resp.userData = ["you've", "been", "owned"];
        resp.globalData = ["ha", "ha", "ha"];
        req.makeReply(resp) >> [];
    }
}

/**
 * @ignore
 */
fl_respondToRequest << [{'flRequest'::}];

/**
 * @ignore
 */
fl_handleButtonEvent = function(msg, sender) {
    if (_fl_curr_viewers[sender.toString()]) {
        if (typeof(_fl_button_callback[msg.vis]) === 'function') _fl_button_callback[msg.vis](msg.fl_button_pressed, msg.data);
    }
}
fl_handleButtonEvent << [{'fl_button_pressed'::}];

/**
 * @ignore
 */
fl_disconnect = function(msg, sender) {
    if (_fl_curr_viewers[msg.disconnect] && sender.toString() == msg.disconnect) {
        _fl_curr_viewers[msg.disconnect] = undefined;
    }
}
fl_disconnect << [{'disconnect'::}];

/**
 * @ignore
 */
fl_updateUserData = function (msg, sender) {
    if (_fl_curr_viewers[sender.toString()] && msg.user == sender.toString()) {
        var cb = _fl_on_user_data_will_update_callback[msg.vis];
        if (typeof(cb) === 'function') {
            if (cb(msg.userData) === false) return; //if app returns false from callback, user data should not update
        }
        if (typeof(_fl_users) === 'undefined') _fl_users = {};
        _fl_users[msg.user] = msg.userData;
        cb = _fl_on_user_data_did_update_callback[msg.vis];
        if (typeof(cb) === 'function') cb();
    }
}
fl_updateUserData << [{'userData'::}];
