var lastMessages = new Array();
lastMessages.push('');
var last_msg_index = undefined;

function appendMessage(msg)
{
    lastMessages.pop(); // the ''
    lastMessages.push(msg);
    lastMessages.push('');
    last_msg_index = lastMessages.length-1;
}

function displayCommand(msg)
{
    editor.getSession().setValue(msg);
}

function addMessage(msg) {
    results.getSession().setValue( results.getSession().getValue() + msg + '\n' );
}

function runCommand() {
    var command = editor.getSession().getValue();
    // Clear with a timer because we're still getting the \n from the editor on Shift-Enter
    setTimeout( function() { editor.getSession().setValue(''); }, 100);

    addMessage('>>> ' + command);
    appendMessage(command);

    var arg_map = [
        'ExecScript',
        'Command', command
    ];
    chrome.send("event", arg_map);
}

// We track key up and key down to make shift + enter trigger a send
function registerHotkeys(elem) {
    var canon = require("pilot/canon");
    canon.addCommand(
        {
            name: 'run',
            exec: runCommand
        }
    );
    canon.addCommand(
        {
            name: 'history-back',
            exec: editorHistoryBack
        }
    );
    canon.addCommand(
        {
            name: 'history-forward',
            exec: editorHistoryForward
        }
    );


    var HashHandler = require("ace/keyboard/hash_handler").HashHandler;
    var ue = require("pilot/useragent");

    var bindings = require("ace/keyboard/keybinding/default_" + (ue.isMac ? "mac" : "win")).bindings;
    delete bindings.reverse;
    bindings["run"] = "Shift-Return";
    bindings["history-back"] = "Shift-PageUp";
    bindings["history-forward"] = "Shift-PageDown";
    editor.setKeyboardHandler(new HashHandler(bindings));
}

function updateEditorHistory() {
    if (last_msg_index != undefined)
        displayCommand( lastMessages[last_msg_index] );
}

function editorHistoryBack() {
    if (last_msg_index !== undefined && last_msg_index-1 >= 0)
        last_msg_index--;
    updateEditorHistory();
}

function editorHistoryForward() {
    if (last_msg_index !== undefined && last_msg_index+1 < lastMessages.length)
        last_msg_index++;
    updateEditorHistory();
}

function closePrompt() {
    var arg_map = [
        'Close'
    ];
    chrome.send("event", arg_map);
}
