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
	console.log("WHAT THE FUCK");
	var $dialog = $('<div></div>')
	.html(text)
	.dialog({
		closeOnEscape: true,
		draggable: true,
		resizable: true,
		title: title
	});
}

$(document).ready(function() {
	
	var options = {copyClassAttr: true, minWidth: 120, arrowSrc: 'arrow_right.gif', onClick: function(e, menuItem){
		
		$.Menu.closeAll();
		
		var action_clicked = $(this).attr('class');
		
		if(action_clicked == 'action_exit') {
			alert_permanent('Exiting', 'Please wait while Sirikata client exits...');
			setTimeout(function() { 
				chrome.send("ui-action", [action_clicked]);
			}, 1000);
		}
		
		if(action_clicked == 'action_toggle_clock') {
			if(timeClockEnabled) {
				timeClockEnabled = false;
				$('#time-clock').hide();
			} else {
				$('#time-clock').show();
				timeClockEnabled = true;
				timeClock();
			}
		}
		
		if(action_clicked == 'action_toggle_fps') {
			if(fpsDisplayEnabled) {
				fpsDisplayEnabled = false;
				$('#fps-display').hide();
			} else {
				fpsDisplayEnabled = true;
				$('#fps-display').show();
			}
		}

		if(action_clicked == 'action_toggle_render_stats') {
                    renderStatsDisplayEnabled = !renderStatsDisplayEnabled;
                    if (renderStatsDisplayEnabled)
                        $('#render-stats-display').show();
                    else
                        $('#render-stats-display').hide();
		}
		
		if(action_clicked == 'action_cdn_upload') {
			$( "#cdn-choose-file" ).dialog( "open" );
		}

	}};  
	$('#menuone').menu(options);
	
	timeClock();
	
});
