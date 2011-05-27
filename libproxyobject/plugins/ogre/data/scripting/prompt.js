sirikata.ui(
    "Scripter",
    function() {

Scripter = {};

var curEditor = undefined;
var editors = [];

var updateCurEditor = function(selected) {
    if (selected === undefined) {
        var $tabs = $('#edittabs');
        selected = $tabs.tabs('option', 'selected');
    }
    curEditor = editors[selected];
};

var Editor = function(objid) {
    var small_objid = (objid.length < 8 ? objid : objid.slice(0, 7));

    var objid_salt = objid + Math.round(Math.random() * 100000).toString();
    console.log('objid_salt: ' + objid_salt);
    var tabname = 'tab-' + objid_salt;
    var tabeditor = 'tab-' + objid_salt + '-editor';
    var tabresults = 'tab-' + objid_salt + '-results';
    var editor_tab = $('<div class="editorthumb" id="' + tabname + '">' +
                          '<div class="editborder"><div id="' + tabresults + '" class="codeedit"></div></div>' +
                          '<div class="editborder"><div id="' + tabeditor + '" class="codeedit"></div></div>' +
                          '</div>');
    editor_tab.appendTo('#edittabs');
    var $tabs = $('#edittabs');
    var idx = $tabs.tabs('length');
    $tabs.tabs('add', '#' + tabname, small_objid);
    editors[idx] = this;
    $tabs.tabs('select', idx);
    updateCurEditor();

    this.object = objid;
    this.tabname = tabname;

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

    this.editor.focus();
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

    sirikata.event("event", 'ExecScript', this.object, command);
    sirikata.log('debug', 'Command:', command);
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
            exec: function(env, args, request) { editHistoryBack(); }
        }
    );
    canon.addCommand(
        {
            name: 'history-forward',
            exec: function(env, args, request) { editHistoryForward(); }
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
        this.displayCommand( this.lastMessages[this.last_msg_index] );
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

var findEditorIndex = function(objid) {
    for (var i=0; i<editors.length; i++) {
	if (editors[i].object == objid) {
            return i;
	}
    }
    return null;
};

var addObject = function(objid) {
    // Reinit
    if (editors.length == 0)
        $('#edittabs').tabs({ select: function(event, ui) { updateCurEditor(ui.index); } });

    var edidx = findEditorIndex(objid);
    if (edidx === null) {
        // Didn't find it, create it
        $( "#emerson-prompt-dialog" ).dialog( "open" );
        curEditor = new Editor(objid);
    }
    else {
        $('#edittabs').tabs('select', i);
    }
};
Scripter.addObject = addObject;

var addMessage = function(objid, msg) {
    var edidx = findEditorIndex(objid);
    if (edidx === null) return;
    editors[edidx].addMessage(msg);
};
Scripter.addMessage = addMessage;

var dialogClosed = function() {
	while (editors.length > 0) {
		closePrompt();
	}
};

var closePrompt = function() {
	var selectedIndex = $('#edittabs').tabs('option', 'selected');
	console.log("selected index = " + selectedIndex);
	delete editors[selectedIndex].results;
	delete editors[selectedIndex].editor;
	$('#edittabs').tabs('remove', selectedIndex);
	$("#" + editors[selectedIndex].tabname).remove();
	editors.splice(selectedIndex, 1);
	if (editors.length == 0) {
	    $( "#emerson-prompt-dialog" ).dialog( "close" );
            $('#edittabs').tabs('destroy');
	} else {
		updateCurEditor();
	}
};

var runCommand = function() {
    if (curEditor)
        curEditor.runCommand();
};

var editHistoryBack = function() {
    if (curEditor)
        curEditor.editHistoryBack();
};

var editHistoryForward = function() {
    if (curEditor)
        curEditor.editHistoryForward();
};


        $LAB
            .script("../ace/build/src/ace-uncompressed.js")
            .script("../ace/build/src/theme-dawn.js")
            .script("../ace/build/src/mode-javascript.js").wait();

	$('<div />').attr({id:'emerson-prompt-dialog', title:'Emerson Scripting'})
	    .append($("<div />").attr({id:'edittabs'})
		    .append($("<ul />").attr({id:'edittab_titles'}).append(''))
		   )
	    .appendTo('body');

        sirikata.ui.window(
            '#emerson-prompt-dialog',
            {
		autoOpen: false,
		height: 'auto',
		width: 450,
		modal: true,
		buttons: {
		    Run: runCommand,
		    "Close Tab": closePrompt
		},
		close: dialogClosed,
                position: 'left'
	    }
        );

	var newcsslink = $("<link />").attr({rel:'stylesheet', type:'text/css', href:'../jquery_themes/redmond/jquery-ui-1.8.6.custom.css'})
	$("head").append(newcsslink);
	var newcsslink = $("<link />").attr({rel:'stylesheet', type:'text/css', href:'../scripting/prompt.css'})
	$("head").append(newcsslink);

});