var timeClockEnabled = true;
function timeClock () {
	if(timeClockEnabled) {
		var curTime = new Date();
		var h = curTime.getHours();
		var m = curTime.getMinutes();
		if(m < 10) {
			m = "0" + m;
		}
		var ampm = (h < 12) ? "AM" : "PM";
		if(h > 12) {
			h = h - 12;
		}

		$('#time-clock').html('' + h + ':' + m + ' ' + ampm);

		setTimeout(timeClock, 1000);
	}
}

var fpsDisplayEnabled = true;
function update_fps(num) {
	var formatted = (Math.round(num*10)/10).toFixed(1);
	$("#fps-num").html(formatted);
}

var renderStatsDisplayEnabled = true;
function update_render_stats(batches, tris) {
	var formatted = batches + " batches, " + tris + " triangles";
	$("#render-stats").html(formatted);
}

function alert_permanent(title, text) {
	var $dialog = $('<div></div>')
	.html(text)
	.dialog({
		closeOnEscape: false,
		draggable: false,
		resizable: false,
		beforeClose: function(event, ui) { return false; },
		open: function(event, ui) {
			/* hides close button */
			$(this).parent().children().children('.ui-dialog-titlebar-close').hide();
		},
		title: title
	});
}

function alert_message(title, text) {
	var $dialog = $('<div></div>')
	.html(text)
	.dialog({
		closeOnEscape: true,
		draggable: true,
		resizable: true,
		title: title
	});
}



(function() {

    var menu_callbacks = {};

    var menu_options = {
        copyClassAttr: true,
        minWidth: 120,
        arrowClass: 'ui-icon-triangle-1-e',
        onClick: function(e, menuItem) {
	    $.Menu.closeAll();

            // We store the menu item id in class because this is the
            // only property copied into the target element by jquery.menu.
	    var action_clicked = $(this).attr('class');
            var cb = menu_callbacks[action_clicked];
            if (typeof(cb) == "function") {
                cb();
            }
	}
    };
    // Track menu items, the actual entries
    var menu_items = {};
    // Also track entire menus. A menu_item can be turned into an entire menu to
    // generate submenus
    var menus = {};

    /** Create a menu item. This takes an object with parameter settings. The
     *  parameters include:
     *  @param id unique id for this menu item (used as the id
     *         attribute)
     *  @param text text to render in the menu item
     *  @param parent the name of the parent menu item
     *  @param click function to call when this menu item is clicked
     *  @returns an object with a clear() method which can be used to remove the
     *   menu item
     */
    sirikata.ui.menu = function(params) {
        $(document).ready(function() {
            var parent;
            if (params.parent !== undefined) {
                // Child of existing menu or submenu
                parent = menus[params.parent];
                if (parent === undefined) {
                    menus[params.parent] = new $.Menu(menu_items[params.parent]);
                    parent = menus[params.parent];
                }

                // addClass adds the class to the menu item. We use
                // this to find callbacks. We have to use classes
                // because their the only property of the node we have
                // control over
                menu_items[params.id] = new $.MenuItem({src: params.text, addClass: params.id}, menu_options);
                parent.addItem(menu_items[params.id]);
            }
            else {
                // New root menu
                var menu_li = $('<li>' + params.text + '</li>')
                    .appendTo('#sirikata-global-menu')
                    .addClass('menumain')
                    .addClass(params.id); // class is the only attr copied
                menus[params.id] = new $.Menu(menu_li, null, menu_options);
            }

            // In both cases, we want to register a click handler
            menu_callbacks[params.id] = params.click;
        });

        return {
            'clear' : function() {
                sirikata.ui.menu.remove(menu_item_selector(params.id));
            }
        };
    };

    // Static method for removing a menu item by selector
    sirikata.ui.menu.remove = function(selector) {
        $(selector).remove();
    };

    $(document).ready(function() {
        // Then register our default menu items.
        sirikata.ui.menu({
            'id' : 'file',
            'text' : 'File'
        });

        sirikata.ui.menu({
            'id' : 'exit',
            'text' : 'Exit',
            'parent' : 'file',
            'click' : function() {
                alert_permanent('Exiting', 'Please wait while Sirikata client exits...');
		setTimeout(function() {
		    sirikata.__event("ui-action", 'exit');
		}, 1000);
            }
        });


        sirikata.ui.menu({
            'id' : 'options',
            'text' : 'Options'
        });

        sirikata.ui.menu({
            'id' : 'options-display',
            'text' : 'Display',
            'parent' : 'options'
        });
        
        sirikata.ui.menu({
            'id' : 'toggle-clock',
            'text' : 'Toggle Clock',
            'parent' : 'options-display',
            'click' : function() {
                if(timeClockEnabled) {
		    timeClockEnabled = false;
		    $('#time-clock').hide();
		} else {
		    $('#time-clock').show();
		    timeClockEnabled = true;
		    timeClock();
		}
            }
        });

        sirikata.ui.menu({
            'id' : 'toggle-fps',
            'text' : 'Toggle FPS Display',
            'parent' : 'options-display',
            'click' : function() {
                if(fpsDisplayEnabled) {
		    fpsDisplayEnabled = false;
		    $('#fps-display').hide();
		} else {
		    fpsDisplayEnabled = true;
		    $('#fps-display').show();
		}
            }
        });

        sirikata.ui.menu({
            'id' : 'toggle-render-stats',
            'text' : 'Toggle Render Stats Display',
            'parent' : 'options-display',
            'click' : function() {
                renderStatsDisplayEnabled = !renderStatsDisplayEnabled;
                if (renderStatsDisplayEnabled)
                    $('#render-stats-display').show();
                else
                    $('#render-stats-display').hide();
            }
        });

        timeClock();

        // Version number
        $("#sirikata-version-display").html('v' + sirikata.version.string + '-' + sirikata.version.commit.slice(0, 6));
    });

})();