sirikata.ui(
    "Chat",
    function() {

Chat = {};

var username = "";
var pageScroll = function() {
    window.scrollBy(0,1000); // horizontal and vertical scroll increments
};

var filterMessage = function(msg, node) {
    // Filter a message for special characters and convert them to
    // display nicely using html

    // Convert tabs to 4 spaces
    msg = msg.replace('\t', '&nbsp;&nbsp;&nbsp;&nbsp;');

    var sub_msgs = msg.split('\n');

    for(var i = 0; i < sub_msgs.length; i++) {
        var p = document.createElement('p');
        node.appendChild( p );
        p.appendChild( document.createTextNode(sub_msgs[i]) );
    }
};

var addMessage = function(msg) {
    var new_p = document.createElement('div');
    filterMessage(msg, new_p);
    new_p.className = 'command';
    var results_div = document.getElementById('Results');
    results_div.appendChild( new_p );

    setTimeout('pageScroll()',100);
};
Chat.addMessage = addMessage;

var clearCommand = function() {
    var command_area = document.getElementById('Command');
    command_area.value = '';
};

var runCommand = function() {
    var command_area = document.getElementById('Command');
    var command = command_area.value;
    command_area.value = '';

    addMessage(username + ": " + command);

    sirikata.event("chat", 'Chat', username, command);
};

// We track key up and key down to make shift + enter trigger a send
var registerHotkeys = function(evt) {
    console.log('registerHotkeys');
    var command_area = document.getElementById('Command');
    command_area.onkeydown = handleCodeKeyDown;
    command_area.onkeyup = handleCodeKeyUp;

    $('#chat-login').hide();
    $('#chat-log').show();

    username = document.getElementById('username').value;
};

var shift_down = false;
var handleCodeKeyDown = function(evt) {
    if (evt.shiftKey)
        shift_down = true;
    if (evt.keyCode == 13 && shift_down) {
        runCommand();
        // We set a timeout because the addition of the new line occurs *after* this handler
        setTimeout('clearCommand()',1);
    }
};

var handleCodeKeyUp = function(evt) {
    if (evt.shiftKey)
        shift_down = false;
};

var toggleVisible = function() {
    var dialog = $( "#chat-dialog" );
    if (dialog.dialog('isOpen'))
        dialog.dialog('close');
    else
        dialog.dialog('open');
};
Chat.toggleVisible = toggleVisible;


        // UI setup
        $('<div>' +
          ' <div id="chat-login">' +
          '  Username: <input type="text" id="username" name="username"/>' +
          '  <button id="chat-submit-name">Register</button>' +
          ' </div>' +
          ' <div id="chat-log">' +
          '  <div id="Results">' +
          '  </div>' +
          '  <div>' +
          '   <form>' +
          '    <textarea id="Command" name="Command" rows="5" cols="30" ></textarea><br>' +
          '    <button id="chat-run-command" type="button">Send</button>' +
          '   </form>' +
          '  </div>' +
          ' </div>' +
          '</div>').attr({id:'chat-dialog', title:'Chat'}).appendTo('body');

        sirikata.ui.window(
            '#chat-dialog',
            {
	        autoOpen: false,
	        height: 'auto',
	        width: 300,
                height: 400,
                position: 'right'
            }
        );

        $('#chat-log').hide();
        sirikata.ui.button('#chat-submit-name').click(registerHotkeys);
        sirikata.ui.button('#chat-run-command').click(runCommand);

});