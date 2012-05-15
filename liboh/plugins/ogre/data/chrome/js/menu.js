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

    var menu_item_id = function(menu_item_name) {
        return 'sirikata-global-menu-' + menu_item_name;
    };
    var menu_item_selector = function(menu_item_name) {
        return '#' + menu_item_id(menu_item_name);
    };

    var menu_options = {
        copyClassAttr: true,
        minWidth: 120,
        arrowSrc: 'arrow_right.gif',
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
            var parent, main_menu;
            if (params.parent !== undefined) {
                parent = $(menu_item_selector(params.parent));
                main_menu = false;
            }
            else {
                parent = $('#sirikata-global-menu-div');
                main_menu = true;
            }

            menu_callbacks[params.id] = params.click;

            // Find or create the ul for holding this node
            var parent_ul = parent.children('ul');
            if (parent_ul.length == 0)
                parent_ul = $('<ul class="menu"/>').appendTo(parent);
            // Add our li for the menu item
            var menu_li = $('<li>' + params.text + '</li>')
                .appendTo(parent_ul)
                .attr('class', params.id) // class is the only attr copied
                .attr('id', menu_item_id(params.id));
            if (main_menu)
                menu_li.addClass('menumain');
            // And regenerate the menu
            $('#sirikata-global-menu').menu(menu_options);
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
        $('#sirikata-global-menu').menu(menu_options);
    };

    $(document).ready(function() {
        // Do initial menu setup
        $('#sirikata-global-menu').menu(menu_options);

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
            'id' : 'toggle-clock',
            'text' : 'Toggle Clock',
            'parent' : 'options',
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
            'parent' : 'options',
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
            'parent' : 'options',
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