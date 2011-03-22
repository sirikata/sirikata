var curEditor = undefined;
var editors = [];

function updateCurEditor(selected) {
    if (selected === undefined) {
        var $tabs = $('#edittabs').tabs();
        selected = $tabs.tabs('option', 'selected');
    }
    curEditor = editors[selected];
};

Editor = function(objid) {
    var small_objid = (objid.length < 8 ? objid : objid.slice(0, 7));

    var tabname = 'tab-' + objid;
    var tabeditor = 'tab-' + objid + '-editor';
    var tabresults = 'tab-' + objid + '-results';
    $('#edittabs').append('<div id="' + tabname + '">' +
                          '<div class="editborder"><div id="' + tabresults + '" class="codeedit"></div></div>' +
                          '<div class="editborder"><div id="' + tabeditor + '" class="codeedit"></div></div>' +
                          '</div>');
    var $tabs = $('#edittabs').tabs();
    var idx = $tabs.tabs('length');
    $tabs.tabs('add', '#' + tabname, small_objid);
    editors[idx] = this;
    $tabs.tabs('select', idx);
    updateCurEditor();

    this.object = objid;

    var theme = "ace/theme/dawn";
    var JavaScriptMode = require("ace/mode/javascript").Mode;

    this.editor = ace.edit(tabeditor);
    this.editor.setTheme(theme);
    this.editor.getSession().setMode(new JavaScriptMode());
    this.editor.renderer.setShowGutter(false);
    this.editor.$parent = this;

    this.results = ace.edit(tabresults);
    this.results.setTheme(theme);
    this.results.getSession().setMode(new JavaScriptMode());
    this.results.renderer.setShowGutter(false);
    this.results.setReadOnly(true);

    this.registerHotkeys();

    this.lastMessages = new Array();
    this.lastMessages.push('');
    this.last_msg_index = undefined;

    this.submitting = false;
};

Editor.prototype.appendMessage = function(msg) {
    this.lastMessages.pop(); // the ''
    this.lastMessages.push(msg);
    this.lastMessages.push('');
    this.last_msg_index = this.lastMessages.length-1;
};

Editor.prototype.displayCommand = function(msg)
{
    this.editor.getSession().setValue(msg);
};

Editor.prototype.addMessage = function(msg) {
    this.results.getSession().setValue( this.results.getSession().getValue() + msg + '\n' );
};

Editor.prototype.runCommand = function(msg) {
    if (this.submitting) return;

    var command = this.editor.getSession().getValue();
    if (command.length == 0 || command.trim().length == 0) return;

    // Clear with a timer because we're still getting the \n from the editor on Shift-Enter
    this.submitting = true;
    var self = this;
    setTimeout( function() { self.editor.getSession().setValue(''); self.submitting = false; }, 100);

    this.addMessage('>>> ' + command);
    this.appendMessage(command);

    var arg_map = [
        'ExecScript',
        this.object,
        command
    ];
    chrome.send("event", arg_map);
};

// We track key up and key down to make shift + enter trigger a send
Editor.prototype.registerHotkeys = function() {
    var canon = require("pilot/canon");
    canon.addCommand(
        {
            name: 'run',
            exec: function(env, args, request) { runCommand(); }
        }
    );
    canon.addCommand(
        {
            name: 'history-back',
            exec: function(env, args, request) { editorHistoryBack(); }
        }
    );
    canon.addCommand(
        {
            name: 'history-forward',
            exec: function(env, args, request) { editorHistoryForward(); }
        }
    );


    var HashHandler = require("ace/keyboard/hash_handler").HashHandler;
    var ue = require("pilot/useragent");

    var bindings = require("ace/keyboard/keybinding/default_" + (ue.isMac ? "mac" : "win")).bindings;
    delete bindings.reverse;
    bindings["run"] = "Shift-Return";
    bindings["history-back"] = "Shift-PageUp";
    bindings["history-forward"] = "Shift-PageDown";
    this.editor.setKeyboardHandler(new HashHandler(bindings));
};

Editor.prototype.updateEditorHistory = function() {
    if (this.last_msg_index != undefined)
        this.displayCommand( this.lastMessages[last_msg_index] );
};

Editor.prototype.editHistoryBack = function() {
    if (this.last_msg_index !== undefined && this.last_msg_index-1 >= 0)
        this.last_msg_index--;
    this.updateEditorHistory();
};

Editor.prototype.editHistoryForward = function() {
    if (this.last_msg_index !== undefined && this.last_msg_index+1 < this.lastMessages.length)
        this.last_msg_index++;
    this.updateEditorHistory();
};

Editor.prototype.closePrompt = function() {
    var arg_map = [
        'Close',
        this.object
    ];
    chrome.send("event", arg_map);
};

editor_inited = false;

function addObject(objid) {
    if (!editor_inited) {
        $('#edittabs').tabs({ select: function(event, ui) { updateCurEditor(ui.index); } });
        editor_inited = true;
    }

    curEditor = new Editor(objid);
}

function addMessage(objid, msg) {
    if (curEditor)
        curEditor.addMessage(msg);
}

function closePrompt() {
    if (curEditor)
        curEditor.closePrompt(msg);
}

function runCommand() {
    if (curEditor)
        curEditor.runCommand();
}

function editHistoryBack() {
    if (curEditor)
        curEditor.editorHistoryBack();
};

function editHistoryForward() {
    if (curEditor)
        curEditor.editorHistoryForward();
};
