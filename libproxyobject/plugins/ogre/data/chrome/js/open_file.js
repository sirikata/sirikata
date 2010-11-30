directory_list_callback_store = {};
function directory_list_request(result) {
	var path_requested = result.path;
	initial_ajax_settings = directory_list_callback_store[path_requested];
	delete directory_list_callback_store[path_requested];
	
	var json_result = [];
	for(var i in result.results) {
		var to_add = {
			data: result.results[i].path,
			attr: {id: path_requested + (path_requested == '/' ? '' : '/') + result.results[i].path},
		}
		if(result.results[i].directory) {
			to_add.state = 'closed';
		}
		json_result.push(to_add);
	}
	
	initial_ajax_settings.success.call(initial_ajax_settings.context,json_result,"", null);	
}

$(document).ready(function() {
	
	$( "#open-file-dialog" ).dialog({
		autoOpen: false,
		height: 300,
		width: 350,
		modal: true,
		buttons: {
			"Open": function() {
				var selected_id = $("#jstree").jstree("get_selected").attr("id");
				if(selected_id) {
					$(this).dialog("close");
				}
			},
			Cancel: function() {
				$( this ).dialog( "close" );
			}
		},
		close: function() {

		},
		open: function(event, ui) {
			$("#jstree")
			.bind("loaded.jstree", function (event, data) {
				
			})
			.jstree({
				core : {  },
				"json_data" : {
					"data" : [{data:'/', attr:{id:'/'}, state:'closed'}],
					"ajax" : {
						"url" : "/",
						"data" : function(n) {
							return n.attr("id");
						}
					},
					"ui" : {
						"select_limit" : 1
					}
				},
				plugins : [ "json_data", "ui", "themeroller", "sort" ]
			});
		}
	});
	
	jQuery.ajax = function(ajax_settings) {
		directory_list_callback_store[ajax_settings.data] = ajax_settings;
		chrome.send("ui-action", ['action_directory_list_request', ajax_settings.data]);
	};
	
});
