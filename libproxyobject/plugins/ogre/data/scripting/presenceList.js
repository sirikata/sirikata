// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

sirikata.ui(
    "PresenceList",
    function() {
	
PresenceList = {};
var selected = undefined;
var window = undefined;
var listSize = 0;

var toggleVisible = function() {
    window.toggle();
};
PresenceList.toggleVisible = toggleVisible;

//add radio button corresponding to obj being added 
var addElem = function(new_obj) {
	var small_objid = (new_obj.length < 8 ? new_obj : new_obj.slice(0, 7));
	var index = listSize++;
	var elemName = 'elem-' + index;
	var newElem = $('<input type="radio" name = "radio" id="'+ elemName +'" value = "' + index + '"/><label for="' +elemName + '">' + small_objid +'</label><br/>');
	$( '#listelem' ).buttonset('destroy');
	$('#listelem').append(newElem); 
	$( '#listelem' ).buttonset();
};
PresenceList.addElem = addElem;


	$('<div id = "presence-list-ui"/>')
		.append($('<form><div id = "listelem"/></form>'))
			.append($('<div id = "list-buttons">' + 
					//' <button id="clear-selected">Clear Selected</button>' +
					' <button id="clear-all">Clear All</button>' +
					' <button id="script-selected">Script Selected</button>' +
					' </div>').append(''))
			.appendTo('body');
	
	window = new sirikata.ui.window(                     // (2)
        "#presence-list-ui",
        {
			autoOpen: false,
			width: 450,
            height: 'auto'
		}
	);
				
	$( '#listelem' ).buttonset();
	$( '#listelem' ).click(setSelected);
	sirikata.ui.button('#clear-all').click(clearAll);
	//sirikata.ui.button('#clear-selected').click(clearSelected);
	sirikata.ui.button('#script-selected').click(scriptSelected);
		
	//sets index of object in list of objects
	function setSelected() {
		selected_index = $('<input[name=radio]:checked', '#listelem').val();
		sirikata.event("defaultTest",selected_index);
	};
	
	//clear all from scripted list
	function clearAll() {
		$('#listelem').empty();
		$('#listelem').buttonset();
		selected_index = undefined;
		sirikata.event("clearAll");
	};
	
	/* function clearSelected() {	
		sirikata.event("defaultTest",selected_index);
		if (selected_index){
			$( '#listelem' ).buttonset('destroy');
			$( '#elem-"'+ selected_index + '"').remove();
			$( '#listelem' ).buttonset();
			sirikata.event("clearSelected",selected_index);
			selected_index = undefined;
		}
	}; */
	
	//sends info from UI back to presenceList
	function scriptSelected() {
		sirikata.event("defaultTest",selected_index);
		if (selected_index){
			sirikata.event("scriptSelected",selected_index);
		} 
	};
});