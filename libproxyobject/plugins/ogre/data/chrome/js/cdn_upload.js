$(document).ready(function() {
	
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
