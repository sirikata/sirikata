/*
 * HTML for villageBuilder UI
 */

sirikata.log("info","\nIN villageBuilderHTML.js\n");

var html = '<div id="village-builder" title="Map">'  +
    '<style type="text/css">' +
       '#gui_grid {' +
            'display: none;' +
       '}' +

       'div.center {' +   
            'position: absolute;' +
            'width: 420px' +
            'top: 10px;' +
            'left: 160px;' +
       '}' +
       
       '#grid_layer, #street_layer, #facility_layer {' +
            'position: absolute;' +
            'top: 0px;'+
            'left: 10px'+ 
       '}' +

        '#facility_layer {' +
            'display: none;'+
            'z-index: 100;'+
        '}'+
        '#gui_facilities {' +
            'position: absolute;' +
            'width: 300px;' +
        '}' +

        '#village_init_info {' +
            'position: absolute;' +
            'left: 0px;' +
            'top: 0px;' +
            'margin: 10px;' +
        '}' +

        '#village_street_templates {' +
            'position: absolute;' +
            'left: 10px;' +
            'top: 120px;' +
            'display: none;' +
        '}'+

        '#village_pos_input {' +
            'position: absolute;' + 
            'left: 10px;' + 
            'top: 20px;' +
        '}' +

       '#village_dimension_input {' +
            'position: absolute;' +
            'left: 10px;' +
            'top: 150px;' +
        '}' +

        '#village_init_bttn {' +
            'position: absolute;' + 
            'left: 10px;' + 
            'top: 250px;' +
        '}' +
    
        '#drawing_tools {'+
            'position: absolute;'+
            'left: 5px;'+
            'top: 0px;'+
            'display: none;'+
        '}'+

        '#eraser, #eraser_selected {' +
            'position: absolute;'+
            'left: 0px;'+
            'top: 0px;'+
        '}' +

        '#eraser_selected {' +
            'display: none;'+
        '}'+

        '#place_car_bttn {' +
            'position: absolute;'+
            'left: 80px;'+
            'top: 10px;'+
            'display: none;'+
        '}'+

        '#village_displaystreets_bttn {' +
            'position: absolute;' +
            'left: 170px;' +
            'top: 450px;' +
        '}' +

        /* ---------- GUI FACILITIES ----------- */

        '#gui_facilities {' +
            'position: absolute;' +
            'left: 10px;' +
            'top: 120px;' +
            'display: none;' +
        '}' +

        '#gui_facilities ul {' +
            'list-style-type: none;' +
            'z-index:10;'+
        '}' +

        '#gui_facilities a {' +
            'text-decoration: none;'+
            'text-align: center;'+
            'width: 100px;'+
            'height: 100px;'+
            'overflow: hidden;'+
        '}' +

        '#gui_facilities label {' +
            'display: block;'+
        '}'+

        '#gui_facilities a img {' +
            'display: block;'+
            'width: 60px;'+
        '}'+
    
        '#gui_facilities button {' +
            'position: absolute;' +
            'left: 310px;'+
            'top: 330px;'+
        '}'+

    '</style>' +
//List of city features
    '<div id="gui_grid" class="center">' + 
        '<canvas id="grid_layer" width="400" height="400"></canvas>' +
        '<canvas id="street_layer" width="400" height="400"></canvas>' +
        '<canvas id="facility_layer" width="400" height="400"></canvas>' +
        '<button id="village_displaystreets_bttn">Place Streets</button>' +
    '</div>' + // end gui_grid and gui_grid_wrapper
    
    '<div id="gui_facilities">' +
        '<ul>' +
            '<li><a class="facility" id="house" href="#">'+
                '<img src="http://dl.dropbox.com/u/16251528/house.png" />' +
                '</a></li>'+
            '<li><a class="facility" id="apartment" href="#">'+
                '<img src="http://dl.dropbox.com/u/16251528/apartment.png" />'+
                '</a></li>'+
            '<li><a class="facility" id="firestation" href="#">' +
                '<img src="http://dl.dropbox.com/u/16251528/firestation.png" />' +
                '</a></li>'+
            '<li><a class="facility" id="school" href="#">'+
                '<img src="http://dl.dropbox.com/u/16251528/school.png" />'+
                '</a></li>' +
        '</ul>' +
        '<button id="village_displayfacilities_bttn">Display Facilities</button>'+
    '</div>'+ // end gui_facilities

    '<div id="village_street_templates">' +
        '<img id="pinhead" class="template" src="http://dl.dropbox.com/u/16251528/pinhead.png" />' +
    '</div>'+

    '<div id="village_init_info">' + 
        '<div id="village_pos_input">'+
            '<label for="x">x</label><input value="-11" id="village_x_pos" type="number" size="10" autofocus /> '+
            '<label for="y">y</label><input value="-13" id="village_y_pos" type="number" size="10" /> '+
            '<label for="z">z</label><input value="5" id="village_z_pos" type="number" size="10" />'+
        '</div>' +
        '<div id="village_dimension_input">'+
            '<label for="rows">Rows:</label> <input id="num_rows" value="6" min="2" step="1" type="number" size="10" />' + 
            '<label for="cols">Cols:</label> <input id="num_cols" value="6" min="2" step="1" type="number" size="10" />'  + 
        '</div>' +
        '<button id="village_init_bttn">Draw Vertex Grid</button>'+
    '</div>' + // end #village_init_info
    
    '<div id="drawing_tools">'+
        '<img id="eraser" src="http://dl.dropbox.com/u/16251528/eraser_big.png"/>'+
        '<img id="eraser_selected" src="http://dl.dropbox.com/u/16251528/eraser_big_selected.png"/>'+
        '<button id="place_car_bttn">Place Car</button>'+
    '</div>'+
'</div>'; // end #village-builder




