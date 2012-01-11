sirikata.ui('DragControls', function() {
    DragControls = {};
    var window;

    function toggleVisible() {
        window.toggle();
    }
    DragControls.toggleVisible = toggleVisible;

    $('<div id="drag-control-ui" title="Drag controls">' +
      '    <div id="axes-mode"><form>' +
      '        <input type="radio" id="move" name="radio" />' +
      '            <label for="move">Move</label>' +
      '        <input type="radio" id="rotate" name="radio" />' +
      '            <label for="rotate">Rotate</label>' +
      '        <input type="radio" id="off" name="radio" checked="checked" />' +
      '            <label for="off">Off</label>' +
      '    </form></div>' +
      '    <div id="axes-snap"><form>' +
      '        <input type="radio" id="local" name="radio" checked="checked" />' +
      '            <label for="local">Local</label>' +
      '        <input type="radio" id="global" name="radio" />' +
      '            <label for="global">Global</label>' +
      '    </form></div>' +
      '    <button id="orient-default-button">Reset orientation</button>' +
      '</div>').appendTo('body');

    window = new sirikata.ui.window(
        '#drag-control-ui',
        {
            width: 190,
            height: 'auto',
            autoOpen: false
        }
    );

    $('#axes-mode').buttonset();
    $('#axes-snap').buttonset();

    function axesMode() {
        if($('#move').attr('checked')) {
            sirikata.event('moveMode');
        } else if($('#rotate').attr('checked')) {
            sirikata.event('rotateMode');
        } else if($('#off').attr('checked')) {
            sirikata.event('offMode');
        }
    }

    function axesSnap() {
        if($('#local').attr('checked')) {
            sirikata.event('snapLocal');
        } else if($('#global').attr('checked')) {
            sirikata.event('snapGlobal');
            $('#global').removeAttr('checked');
            $('#axes-snap').buttonset('refresh');
        }
    }

    function orientDefault() {
        sirikata.event('orientDefault');
    }

    var events = {
        '#axes-mode': axesMode,
        '#axes-snap': axesSnap,
        '#orient-default-button': orientDefault
    };

    for(var id in events) {
        sirikata.ui.button(id).click(events[id]);
    }

});
