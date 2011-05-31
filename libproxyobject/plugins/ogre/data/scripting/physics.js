sirikata.ui(
    "PhysicsSettings",
    function() {


PhysicsSettings = {}; // External interface
var window; // Our UI window

var toggleVisible = function() {
    var dialog = $( "#physics-dialog" );
    if (dialog.dialog('isOpen'))
        dialog.dialog('close');
    else
        dialog.dialog('open');
};
PhysicsSettings.toggleVisible = toggleVisible;


// FIXME having to list these stinks, but we can't pass objects through currently
var setUIInfo = function(treatment, mesh, mass) {
    // Clear all options
    $('.treatment input').removeAttr('checked');
    $('.collision-mesh input').removeAttr('checked');

    // Set values
    $('.treatment #' + treatment).attr('checked', 'checked');
    $('.collision-mesh #' + mesh).attr('checked', 'checked');
    $('#massvalue').val(mass);

    // Refresh the UI based on new settings
    $( "#treatment" ).buttonset('refresh');
    $( "#collision-mesh" ).buttonset('refresh');

    // And make sure that the window is enabled since we have a valid object now
    window.enable();
};
PhysicsSettings.setUIInfo = setUIInfo;

var disable = function(settings) {
    window.disable();
};
PhysicsSettings.disable = disable;

var setScriptInfo = function() {
    // Sends info from UI back to the script.
    // Note that we use ui-state-active because the buttonset doesn't
    // seem to update the underlying input radio's properly. Since
    // that gets us the lable, we use the 'for' attr
    var treatment = $('.treatment label.ui-state-active').attr('for');
    var collision_mesh = $('.collision-mesh label.ui-state-active').attr('for');
    var mass = $('#massvalue').val();
    sirikata.event('requestPhysicsUpdate', treatment, collision_mesh, mass);
};

        // UI setup
        $('<div id="physics-dialog">' +
          ' <div id="treatment" class="treatment">' +
          ' <form>' +
	  '  <input type="radio" id="ignore" name="radio" checked="checked" /><label for="ignore">Disabled</label>' +
	  '  <input type="radio" id="static" name="radio" /><label for="static">Static</label>' +
	  '  <input type="radio" id="dynamic" name="radio" /><label for="dynamic">Dynamic</label>' +
          ' </form>' +
          ' </div>' +
          ' <div id="collision-mesh" class="collision-mesh">' +
          ' <form>' +
	  '  <input type="radio" id="sphere" name="radio" checked="checked" /><label for="sphere">Bounding Sphere</label>' +
	  '  <input type="radio" id="box" name="radio" /><label for="box">Bounding Box</label>' +
	  '  <input type="radio" id="triangles" name="radio" /><label for="triangles">Triangles (static only)</label>' +
          ' </form>' +
          ' </div>' +
          ' <div id="mass">' +
          ' <form>' +
	  '  <input type="number" id="massvalue" value="1.0" /><label for="massvalue">Mass</label>' +
          ' </form>' +
          ' </div>' +
          '</div>').attr({title:'Physics'}).appendTo('body');

        window = new sirikata.ui.window(
            '#physics-dialog',
            {
	        height: 'auto',
	        width: 400,
                autoOpen: false
            }
        );

        $( "#treatment" ).buttonset();
        $( "#collision-mesh" ).buttonset();

        $( '#treatment, #collision-mesh' ).click(setScriptInfo);
        $( '#massvalue' ).change(setScriptInfo);

        // Disable by default
        disable();
});
