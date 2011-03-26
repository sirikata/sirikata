$(document).ready(function() {
	
	$('<div />').attr({id:'cdn-upload-dialog', title:'CDN Upload Tool'})
		.append($('<p />').attr({id:'cdn-upload-tips'}).addClass('validateTips').append('All form fields are required'))
		.append($('<form />').addClass('ui-widget')
			.append($('<fieldset />').addClass('ui-corner-all ui-widget ui-widget-content')
				.append($('<label />').attr({'for':'resource-title'}).append('Title:'))
				.append($('<input />')
					.attr({type:'text', name:'resource-title', id:'resource-title', value:''})
					.addClass('text ui-widget-content ui-corner-all'))
				.append($('<label />').attr({'for':'resource-description'}).append('Description:'))
				.append($('<textarea />')
					.attr({name:'resource-description', id:'resource-description', rows:'2', cols:'20'})
					.addClass('text ui-widget-content ui-corner-all'))
					.append('')
			)
		)
	.appendTo('body');
		
	/*
	<div id="cdn-upload-dialog" title="CDN Upload Tool">
		<p id="cdn-upload-tips" class="validateTips">All form fields are required.</p>
	
		<form class="ui-widget">
		<fieldset class="ui-corner-all ui-widget ui-widget-content">
			<label for="resource-title">Title</label>
			<input type="text" name="resource-title" id="resource-title" value="" class="text ui-widget-content ui-corner-all" />
			
			<label for="resource-description">Description:</label>
			<textarea id="resource-description" name="resource-description" rows="2" cols="20" class="text ui-widget-content ui-corner-all"></textarea>
		</fieldset>
		</form>
	</div>*/
	
	$('<div />').attr({id:'cdn-choose-file', title:'CDN Upload Tool'})
		.append($('<p />').attr({id:'cdn-choose-file-tips'}).addClass('validateTips').append('Choose a collada file to upload.'))
		.append($('<form />').addClass('ui-widget')
			.append($('<fieldset />').addClass('ui-corner-all ui-widget ui-widget-content')
				.append($('<label />').attr({'for':'resource-file-path'}).append('File Path'))
				.append($('<input />')
					.attr({type:'text', name:'resource-file-path', id:'resource-file-path', value:''})
					.addClass('text ui-widget-content ui-corner-all inline'))
				.append($('<input />')
					.attr({type:'button', name:'resource-file-browse', id:'resource-file-browse', value:'Browse...'})
					.addClass('ui-widget-content ui-corner-all inline'))
			)
		)
	.appendTo('body');
		
	/*<div id="cdn-choose-file" title="CDN Upload Tool">
		<p id="cdn-choose-file-tips" class="validateTips">Choose a collada file to upload.</p>
	
		<form class="ui-widget">
		<fieldset class="ui-corner-all ui-widget ui-widget-content">
			<label for="resource-file-path">File Path</label>
			<input type="text" name="resource-file-path" id="resource-file-path" class="text ui-widget-content ui-corner-all inline" />
			<input type="button" name="resource-file-browse" id="resource-file-browse" class="ui-widget-content ui-corner-all inline" value="Browse..." />
		</fieldset>
		</form>
	</div>*/
		
	$(function() {
	
		var eTitle = $("#resource-title");
		var ePath = $("#resource-file-path");
		var eDescription = $("#resource-description");
		var allFields = $([]).add(eTitle).add(ePath).add(eDescription);
		var tips = $("#cdn-upload-tips");

		function updateTips( t ) {
			tips
				.text( t )
				.addClass( "ui-state-highlight" );
			setTimeout(function() {
				tips.removeClass( "ui-state-highlight", 1500 );
			}, 500 );
		}
	
		function checkLength( o, n, min, max ) {
			if ( o.val().length > max || o.val().length < min ) {
				o.addClass( "ui-state-error" );
				updateTips( "Length of " + n + " must be between " +
					min + " and " + max + "." );
				return false;
			} else {
				return true;
			}
		}
		
		function checkRegexp( o, regexp, n ) {
			if ( !( regexp.test( o.val() ) ) ) {
				o.addClass( "ui-state-error" );
				updateTips( n );
				return false;
			} else {
				return true;
			}
		}
		
		$( "#cdn-upload-dialog" ).dialog({
			autoOpen: false,
			height: 'auto',
			width: 450,
			modal: true,
			buttons: {
				"Upload": function() {
					var bValid = true;
					allFields.removeClass( "ui-state-error" );
	
					bValid = bValid && checkLength( eTitle, "Title", 1, 100 );
					bValid = bValid && checkLength( eDescription, "Description", 1, 1000 );
					bValid = bValid && checkLength( ePath, "File Path", 1, 200 );
	
					//bValid = bValid && checkRegexp( name, /^[a-z]([0-9a-z_])+$/i, "Username may consist of a-z, 0-9, underscores, begin with a letter." );
	
					if ( bValid ) {
						$( this ).dialog( "close" );
					}
				},
				Cancel: function() {
					$( this ).dialog( "close" );
				}
			},
			close: function() {
				allFields.val("").removeClass( "ui-state-error" );
			}
		});
	
	});
	
	$(function() {
		
		var ePath = $("#resource-file-path");
		var allFields = $([]).add(ePath);
		var tips = $("#cdn-choose-file-tips");

		function updateTips( t ) {
			tips
				.text( t )
				.addClass( "ui-state-highlight" );
			setTimeout(function() {
				tips.removeClass( "ui-state-highlight", 1500 );
			}, 500 );
		}
	
		function checkLength( o, n, min, max ) {
			if ( o.val().length > max || o.val().length < min ) {
				o.addClass( "ui-state-error" );
				updateTips( "Length of " + n + " must be between " +
					min + " and " + max + "." );
				return false;
			} else {
				return true;
			}
		}
		
		function checkRegexp( o, regexp, n ) {
			if ( !( regexp.test( o.val() ) ) ) {
				o.addClass( "ui-state-error" );
				updateTips( n );
				return false;
			} else {
				return true;
			}
		}
		
		$( "#cdn-choose-file" ).dialog({
			autoOpen: false,
			height: 'auto',
			width: 450,
			modal: true,
			buttons: {
				"Next": function() {
					var bValid = true;
					allFields.removeClass( "ui-state-error" );
	
					bValid = bValid && checkLength( ePath, "File Path", 1, 200 );

					if ( bValid ) {
						window.alert("You chose " + ePath.val());
						$( this ).dialog( "close" );
					}
				},
				Cancel: function() {
					$( this ).dialog( "close" );
				}
			},
			close: function() {
				allFields.val("").removeClass( "ui-state-error" );
			}
		});
	
	});
	
	$("#resource-file-browse").click(function() {
		$("#open-file-dialog").bind("dialogclose",function(event, ui) {
			var selected_id = $("#jstree").jstree("get_selected").attr("id");
			if(selected_id) {
				$("#resource-file-path").val(selected_id);
			}
			$(this).unbind(event);
		});
		$("#open-file-dialog").dialog("open");
	});
	
});
