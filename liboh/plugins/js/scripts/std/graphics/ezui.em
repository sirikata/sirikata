var _ezui_global = {};

/* _ezui_users maps USERS' PRESENCE IDS to HASHES OF VARNAMES -> DATA. */
var _ezui_users = {};
var _ezui_scripts = {}; /* maps presence ids to scripts*/
var _ezui_curr_viewers = {}; /*hash of avatar visible IDs viewing this UI -> value true if currently viewing*/
var _ezui_on_connection_callback = {};
var _ezui_on_user_data_did_update_callback = {};
var _ezui_on_user_data_will_update_callback = {};
var _ezui_button_callback = {};
var _ezui_gdata_update_timer;
var _ezui_rickroll = @write('<iframe width="425" height="349" src="http://www.youtube.com/embed/oHg5SJYRHA0" frameborder="0" allowfullscreen></iframe>')@;

var ezui = {};

ezui.script = function (s, me, clearData) {
    me = (me || system.self).toString();
    if (clearData) {
        _ezui_users = {};
        _ezui_global = {};
    }
    _ezui_scripts[me] = s;
}

ezui.exec = function(s) {
    ezui.sendAllViewers({uieval: s});
}

ezui.sendAllViewers = function (msg) {
    for (v in _ezui_curr_viewers) {
        var isViewer = _ezui_curr_viewers[v];
        if (isViewer) {
            msg >> system.createVisible(v) >> [];
        }
    } 
}

ezui.onButtonPressed = function(func, me) {
    if (typeof(func) === 'function') {
        me = (me || system.self).toString();
        _ezui_button_callback[me] = func;
    }
}

ezui.onConnection = function(func) {
    me = (me || system.self).toString();
    _ezui_on_connection_callback[me] = func;
}

ezui.onUserDataDidChange = function (func, me) {
    me = (me || system.self).toString();
    _ezui_on_user_data_did_update_callback[me] = func;
}

ezui.onUserDataWillChange = function (func, me) {
    me = (me || system.self).toString();
    _ezui_on_user_data_will_update_callback[me] = func;
}


ezui.userData = function (user, key, me) {
    //me = (me || system.self).toString();
    user = user.toString();
    if (typeof(_ezui_users) === 'undefined') _ezui_users = {};
    if (typeof(_ezui_users[user]) === 'undefined') _ezui_users[user] = {};
    return _ezui_users[user][key];
}

ezui.iterateUserData = function (func, me) {
    //me = (me || system.self).toString();
    for (u in _ezui_users) {
        if (_ezui_users.hasOwnProperty(u)) {
            func(_ezui_users[u]);
        }
    }
}

ezui.globalData = function (key, me) {
    //me = (me || system.self).toString();
    if (typeof(_ezui_global) === 'undefined') _ezui_global = {};
    return _ezui_global[key];
}

ezui.setGlobalData = function (key, value, me) {
    if (typeof(_ezui_global) === 'undefined') {
        _ezui_global = {};
    }
    _ezui_global[key] = value;
    if (_ezui_gdata_update_timer) {
        _ezui_gdata_update_timer.suspend();
    }
    _ezui_gdata_update_timer = system.timeout(0.1, ezui_sendGdataUpdateMsg);
}

ezui_sendGdataUpdateMsg = function () {
    var msg = {};
    msg.updatedGlobalData = _ezui_global;
    ezui.sendAllViewers(msg);
}

ezui_respondToRequest = function (req, sender) {
    var resp = {};
    if (sender.toString() == req.sender) {
        //valid request
        resp.self = system.self;
        resp.script = _ezui_scripts[req.recipient];
        
        resp.userData = _ezui_users[req.sender];
        if (typeof(resp.userData) === 'undefined') resp.userData = {};
        
        resp.globalData = _ezui_global;
        if (typeof(resp.globalData) === 'undefined') resp.globalData = {};
        
        _ezui_curr_viewers[req.sender] = true;
        if (typeof(_ezui_on_connection_callback[req.recipient]) === 'function') {
            _ezui_on_connection_callback[req.recipient](req.sender);
        }
        req.makeReply(resp) >> [];
    } else {
        //user is trying to be funny.  Rickroll them.
        resp.script = _ezui_rickroll;
        resp.userData = ["you've", "been", "owned"];
        resp.globalData = ["ha", "ha", "ha"];
        req.makeReply(resp) >> [];
    }
}
ezui_respondToRequest << [{'ezuiRequest'::}];

ezui_handleButtonEvent = function(msg, sender) {
    if (_ezui_curr_viewers[sender.toString()]) {
        if (typeof(_ezui_button_callback[msg.vis]) === 'function') _ezui_button_callback[msg.vis](msg.ezui_button_pressed);
    }
}
ezui_handleButtonEvent << [{'ezui_button_pressed'::}];

ezui_disconnect = function(msg, sender) {
    if (_ezui_curr_viewers[msg.disconnect] && sender.toString() == msg.disconnect) {
        _ezui_curr_viewers[msg.disconnect] = undefined;
    }
}
ezui_disconnect << [{'disconnect'::}];

ezui_updateUserData = function (msg, sender) {
    if (_ezui_curr_viewers[sender.toString()] && msg.user == sender.toString()) {
        var cb = _ezui_on_user_data_will_update_callback[msg.vis];
        if (typeof(cb) === 'function') cb();
        if (typeof(_ezui_users) === 'undefined') _ezui_users = {};
        _ezui_users[msg.user] = msg.userData;
        cb = _ezui_on_user_data_did_update_callback[msg.vis];
        if (typeof(cb) === 'function') cb();
    }
}
ezui_updateUserData << [{'userData'::}];
