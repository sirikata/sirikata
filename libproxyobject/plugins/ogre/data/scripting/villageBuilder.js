/* KNOWN BUGS
 **********************************************************************
 * can't double-drag items into grid
 **********************************************************************
 */


sirikata.ui(
    "VillageBuilder",
    function() {
        

VillageBuilder = {}; // External interface

// global vars
var window; // Our UI window
var ctx_grid = null; // canvas context
var ctx_street = null;
var ctx_facility = null; // facility-level canvas context
var space = 0; // spacing between gridlines
var cols, rows;
var grid_bg_color = "#78b354"; // background color of grid
var gridline_color = "#96bd7f"; // color of path (street) on grid
var margin_left;
var margin_right;

// width and height of grid and ui window
var grid_width;
var grid_height;
var window_width = 580;
var window_height = 600;

// street layer tools
var streets = [];
var erase = false;

// facility layer tools
var facilities = [];
var facilityImages = {}; // images that you can drag to place on grid
facilityImages['house'] = "http://dl.dropbox.com/u/16251528/house.png";
facilityImages['firestation'] = "http://dl.dropbox.com/u/16251528/firestation.png";
facilityImages['school'] = "http://dl.dropbox.com/u/16251528/school.png";
facilityImages['apartment'] = "http://dl.dropbox.com/u/16251528/apartment.png";
var arrows = {}; // arrows that you can click to change orientation of facility
arrows['north'] = "http://dl.dropbox.com/u/16251528/arrow-north.png";
arrows['east'] = "http://dl.dropbox.com/u/16251528/arrow-east.png";
arrows['west'] = "http://dl.dropbox.com/u/16251528/arrow-west.png";
arrows['south'] = "http://dl.dropbox.com/u/16251528/arrow-south.png";


/*
 * Switches from grid view to facilities view,
 * hiding draggable templates, disabling grid-line drawing,
 * and showing draggable facilities
 */
function switch_to_facilities_view() {
    $("#village_street_templates").hide();    
    $("#village_displaystreets_bttn").hide();
    $("#gui_facilities").show();
    $("#facility_layer").show();
    $("#place_car_bttn").show();
    canvas_facility_layer_init();
}

/*
 * Loops over the global array, facilities, and calls a sirikata
 * event which displays the facilities in the appropriate positions
 * in the world
 */
function village_displayfacilities() {
    for(var i = 0; i < facilities.length; i++) {
        sirikata.event("draw_village_facility",facilities[i].type,facilities[i].direction,facilities[i].col,facilities[i].row);
    }

    canvas_draw_facilities(ctx_grid); // draws all the facilities in the grid-line layer so that the user can't modify already-displayed facilities
    facilities.length = 0; // once all the facilities in the array are drawn, clears array so that additional facilities can be added to world
    canvas_facility_layer_clear(); // clears the facility icons in the facility layer so that the user can add new ones
 
}


/*------------------------- CANVAS DISPLAY OPERATIONS -------------------------*/

/*
 * Converts mouse event coordinates in the window to coordinates on the grid
 * Right now this is hard-coded. Needs fixes so that it becomes more robust
 */
function convert_to_canvas_coord(pos) {
    return {x:(pos.left-140),y:(pos.top+30)};
}

/*
 * Initializes the grid-line canvas so that a green rectangle
 * with the right number of rows and columns appears in the ui window
 */
function canvas_init() {
    // dimentions of the grid
    grid_width = cols*space;
    grid_height = rows*space;
    

    // color and place grid bg on canvas
    ctx_grid = $("#grid_layer")[0].getContext("2d");
    ctx_grid.fillStyle=grid_bg_color;
    margin_left = (400-grid_width)/2;
    margin_top = (400-grid_height)/2;

    ctx_grid.fillRect(margin_left,margin_top,grid_width,grid_height);

    //draw gridlines
    canvas_draw_gridlines();
    
    // init ctx_street
    canvas_street_layer_update();    
    canvas_street_layer_add_mouse_events();
}

/*
 * When called, clears the street-layer canvas (which is above the grid-line 
 * canvas layer) and redraws the streets after the user adds or removes a street 
 */
function canvas_street_layer_update() {
    ctx_street = $("#street_layer")[0].getContext("2d");
    ctx_street.clearRect(0,0,400,400); // clears street-layer
    
    // loops over streets in the global array, streets, and draws each one
    for(var i = 0; i < streets.length; i++) {
        var st = streets[i];
        gui_grid_drawstreet(st.c1,st.r1,st.c2,st.r2);
    }
}

/*
 * When the user clicks on a facility icon on the facility layer,
 * highlights the facility icon
 */
function facility_highlight(x,y,size) {
    var margin = space*0.05;
    ctx_facility.globalAlpha = 1;
    ctx_facility.strokeStyle = "#ffff00";
    ctx_facility.lineWidth = space*0.05;
    ctx_facility.lineJoin = "round";
    ctx_facility.strokeRect(x-margin, y-margin, size+2*margin, size+2*margin);
}

/*
 * Given a canvas layer as a parameter, draws the facility icons on
 * the canvas layer.
 */
function canvas_draw_facilities(context) {
    for(var i = 0; i < facilities.length; i++) {
        var f = facilities[i];
        if(f.type=="school") var size = space*1.8; // hard-coded for now - school takes up more space
        else var size = space*0.8;
        var padding = space*0.1;
        var x = (f.col)*space + margin_left;
        var y = (f.row)*space + margin_top;
        context.globalAlpha = 1;
        context.drawImage(f.img,x+padding,y+padding,size,size);
        canvas_draw_arrows(f.direction,f.arrows,context,x+space/2,y+space/2);
    }
}

/*
 * Given the direction of the arrow that should be highlighted, draws four
 * arrows that represents the four directions the facility could be facing
 */
function canvas_draw_arrows(direction,arrows,context, x, y) {
       var size = space/4;

        // north
        if(direction=="north") context.globalAlpha = 0.9;
        else context.globalAlpha = 0.3;
        context.drawImage(arrows.north,x-(size/2),y-(space/2),size,size);

        // south
        if(direction=="south") context.globalAlpha = 0.9;
        else context.globalAlpha = 0.3;
        context.drawImage(arrows.south,x-(size/2),y+(space/2)-size,size,size);

        // west
        if(direction=="west") context.globalAlpha = 0.9;
        else context.globalAlpha = 0.3;
        context.drawImage(arrows.west,x-(space/2),y-(size/2),size,size);

        // east
        if(direction=="east") context.globalAlpha = 0.9;
        else context.globalAlpha = 0.3;
        context.drawImage(arrows.east,x+(space/2)-size,y-(size/2),size,size);

}

/*
 * Adds mouse events and defines relevant function that helps
 * repond to mouse events to the facility canvas layer
 */
function canvas_facility_layer_add_mouse_events() {
    var selected = null; // points to a facility object if it is selected

    /*
     * Updates the facility layer by clearing every facility icon and
     * redrawing each after the user has added, erased, or moved a facility
     */
    function canvas_facility_layer_update() {
        canvas_facility_layer_clear();
        if(selected!=null) { // highlight selected facility
            var padding = space*0.1;
            var x = margin_left + (selected.col)*space + padding;
            var y = margin_top + (selected.row)*space + padding;
            if(selected.type=="school") var size = space*1.8;
            else var size = space*0.8;
            facility_highlight(x,y,size);
        }
        canvas_draw_facilities(ctx_facility); // re-draw all facilities on canvas
        
    }

    /*
     * Given mouse coordinates relative to the canvas,
     * returns an object containing information on the facility
     * that rests at the coordinate or returns null if none
     * is found
     */
    function findFacilityAt(x,y) {

        var col = Math.floor((x-margin_left)/space);
        var row = Math.floor((y-margin_top)/space);

        for(var i = 0; i < facilities.length; i++) {
            var f = facilities[i];
            if(f.col == col &&
               f.row == row) {
                return {facility:f,index:i};
            }
            if(f.type == "school") {
                if(col >= f.col && col <= f.col+1 &&
                   row >= f.row && row <= f.row+1) {
                    return {facility:f,index:i};
                }
            }
            
        }
        return null;
    }

    /*
     * Given mouse coordinates relative to the canvas and
     * the length of the side of the square that the arrows are
     * contained in (intercept), returns the direction that the
     * user clicked on
     */
    function findDirectionAt(x,y,intercept) {
        var offset_x = (x-margin_left)%space;
        var offset_y = (y-margin_top)%space;
        if(offset_x > offset_y) {
            if(-offset_x+intercept < offset_y) return "east";
            return "north";
        } else {
            if(-offset_x+intercept < offset_y) return "south";
            return "west";
        }
    }

    /*
     * Given a mouseEvent e, calculates position of the cursor,
     * checks if the position means anything, and acts accordingly
     */
    function select(e) {
        var pos = getCursorPosition(e);
        var f = findFacilityAt(pos.x,pos.y);
        if(f == null) {
            selected = null;
            return;
        }
        
        if(erase) { // if in erase mode, get rid of facility
            facilities.splice(f.index,1);
        } else { // if facility is selected in non-erase mode, select and change direction if necessary
            selected = f.facility;
            changedir(pos.x,pos.y,f.facility);
        }

        canvas_facility_layer_update(); // update facility-layer canvas to reflect any changes
    }

    /*
     * If a facility is selected and user is dragging the mouse,
     * move the facility to a new location by snapping to the squares
     * on the grid
     */
    function drag(e) {
        if(selected==null) return;
        var pos = getCursorPosition(e);
        
        var col = Math.floor((pos.x-margin_left)/space);
        var row = Math.floor((pos.y-margin_top)/space);
        if(selected.type == "school") { // hard-coded; school is special because it takes up more grid space
           if((col<=selected.col+1 && col>=selected.col) &&
              (row<=selected.row+1 && row>=selected.row)) {
               return;
           }
        }
        selected.col = col;
        selected.row = row;
        canvas_facility_layer_update();
    }

    function changedir(x,y,f) {
        var intercept = space;
        var dir = findDirectionAt(x,y,intercept);
        f.direction = dir;
        canvas_facility_layer_update();
    }

    $("#facility_layer")[0].addEventListener("mousedown",select,false);
    $("#facility_layer")[0].addEventListener("mousemove",drag,false);
    $("#facility_layer")[0].addEventListener("mouseup",function(){
        selected=null;
        canvas_facility_layer_update();
    },false);
}

function canvas_facility_layer_clear() {
    ctx_facility = $("#facility_layer")[0].getContext("2d");
    ctx_facility.clearRect(0,0,400,400);
}

function canvas_facility_layer_init() {
    canvas_facility_layer_clear();
    canvas_facility_layer_add_mouse_events();
}
function getCursorPosition(e) {
    var x;
    var y;
    if(e.pageX != undefined && e.pageY != undefined) {
        x = e.pageX;
        y = e.pageY;
    } else {
        x = e.clientX + document.body.scrollLeft +
            document.documentElement.scrollLeft;
        y = e.clientY + document.body.scrollTop +
            document.documentElement.scrollTop;
    }
    
    x -= 175;// $("#grid")[0].offsetLeft;
    y -= 40;//$("#grid")[0].offsetTop;
    
    return {x:x,y:y};
}

function canvas_draw_gridlines() {
    ctx_grid.strokeStyle=gridline_color;
    ctx_grid.lineWidth=1;
   
    ctx_grid.beginPath();
    for(var x = margin_left; x <= grid_width + margin_left; x += space){
        for(var y = margin_top; y <= grid_height + margin_top; y += space){
            // adjusted x and y value for when line is on edge
            var ax = x;
            var ay = y; 
            if(x == margin_left) ax = x + 0.5;
            if(y == margin_top) ay = y + 0.5;
            if(x == grid_width + margin_left) ax = x - 0.5;
            if(y == grid_height + margin_top) ay = y - 0.5;
            
            ctx_grid.moveTo(margin_left, ay);
            ctx_grid.lineTo(grid_width + margin_left,ay);
            ctx_grid.moveTo(ax, margin_top);
            ctx_grid.lineTo(ax, grid_height + margin_top);
        }
    }
    ctx_grid.stroke();
    ctx_grid.closePath();
}

function gui_grid_drawstreet(col1,row1,col2,row2) {
    ctx_street.lineWidth = 0.1*space;
    ctx_street.strokeStyle = "#000000";
    ctx_street.beginPath();

    var epsilon = 0.05*space;
    var startx = col1*space + margin_left;
    var starty =row1*space + margin_top;
    var endx = col2*space + margin_left;
    var endy = row2*space + margin_top;

    if(col1==col2) {
        if(row1>0) starty -= epsilon;
        if(row2<rows) endy += epsilon;
        if(col1==0 && col2==0) {
            startx += epsilon;
            endx += epsilon;
        } else if(col1==cols && col2==cols) {
            startx -= epsilon;
            endx -= epsilon;
        }
    } else if(row1==row2) {
        if(col1>0) startx -= epsilon;
        if(col2<cols) endx += epsilon;            
        if(row1==0 && row2==0) {
            starty += epsilon;
            endy += epsilon;
        } else if(row1==rows && row2==rows) {
            starty -= epsilon;
            endy -= epsilon;
        }
    }

    ctx_street.moveTo(startx,starty);
    ctx_street.lineTo(endx,endy);
    ctx_street.stroke();
    ctx_street.closePath();
}


function canvas_street_layer_add_mouse_events() {
    var mouse_down = false;

    function getLeftCol(mousex) {
        var prev = null;
        var epsilon = space/8;
        for(var col = 0; col <= cols; col++) {
            if((margin_left + col*space - epsilon) > mousex) {
                return prev;
            }
            prev = col;
        }
        return null;

    }

    function getTopRow(mousey) {
        var prev = null;
        var epsilon = space/8;
        for(var row = 0; row <= rows; row++) {
            if((margin_top + row*space - epsilon) > mousey) {
                return prev;
            }
            prev = row;
        }
        return null;
    }

    function getGridLine(mousex, mousey) {
        // check horizontal street sections
        for(var row = 0; row <= rows; row++) {
            var y = margin_top + space*row;
            var epsilon = space/5;                
            if(mousey <= y+epsilon && mousey >= y-epsilon) {
                var col1 = getLeftCol(mousex);
                if(col1==null) return null;
                var col2 = col1 + 1;
                return {col1:col1,row1:row,col2:col2,row2:row};
            }
        }

        // check vertical street sections
        for(var col = 0; col <= cols; col++) {
            var x = margin_left + space*col;
            var epsilon = space/5;                
            if(mousex <= x+epsilon && mousex >= x-epsilon) {
                var row1 = getTopRow(mousey);
                if(row1 == null) return null;
                var row2 = row1+1;
                return {col1:col,row1:row1,col2:col,row2:row2};
            }
        }

        return null;
    }

    function village_drag_drawstreet(e) {
        if(!mouse_down) return;
        if(!erase) village_drawstreet(e);
        else village_erase_street(e);
    }

    function village_drawstreet(e) {
        var pos = getCursorPosition(e);
        var gridLine = getGridLine(pos.x, pos.y);
        if(gridLine!=null) {
            var col1 = gridLine.col1;
            var row1 = gridLine.row1;
            var col2 = gridLine.col2;
            var row2 = gridLine.row2;
            sirikata.event("extend_village_streets",col1,row1,col2,row2);
            if(gui_grid_find_street_index(col1,row1,col2,row2)==null) {
                streets.push({c1:col1,r1:row1,c2:col2,r2:row2});
                canvas_street_layer_update();
            }
            //gui_grid_drawstreet(col1,row1,col2,row2);
        }
    }

    function gui_grid_find_street_index(c1,r1,c2,r2) {
        for(var i = 0; i < streets.length; i++) {
            if(streets[i].c1==c1 &&
               streets[i].r1==r1 &&
               streets[i].c2==c2 &&
               streets[i].r2==r2) return i;
        }
        return null;
    }

    function village_erase_street(e) {
        var pos = getCursorPosition(e);
        var gridLine = getGridLine(pos.x,pos.y);
        if(gridLine!=null) {
            var col1 = gridLine.col1;
            var row1 = gridLine.row1;
            var col2 = gridLine.col2;
            var row2 = gridLine.row2;
            
            
            var index = gui_grid_find_street_index(col1,row1,col2,row2);
            if(index != null) {
                streets.splice(index,1);
                sirikata.event("erase_village_street",col1,row1,col2,row2);
                canvas_street_layer_update();
            }
        }
    }

    $("#street_layer")[0].addEventListener("mousedown",function(e) {
        mouse_down = true;
    },false);
    $("#street_layer")[0].addEventListener("mouseup",function(e) {
        mouse_down = false;
        if(erase) {
            village_erase_street(e);
        } else {
            village_drawstreet(e);
        }
    },false);
    $("#street_layer")[0].addEventListener("mousemove",village_drag_drawstreet,false);

}

/*--------------------------------- CANVAS DISPLAY OPERATIONS END -------------------------------*/





/*-------------------- BUTTON CALLBACK FUNCTIONS THAT COMMUNICATE TO WORLD ----------------------*/

/* Function: village_init
 * --------------------------
 * Executes the draw island/ street generator when clicked, sets rows and cols according to the numbers entered by the user
 */    
function village_init() {
    $("#village_init_info").hide();
    $("#village_street_templates").show();
    $("#drawing_tools").show();
    var vrows = $('#num_rows').val();
    var vcols = $('#num_cols').val();
    
    var position = {};
    position.x = parseInt($('#village_x_pos').val());
    position.y = parseInt($('#village_x_pos').val());
    position.z = parseInt($('#village_x_pos').val());
    rows = parseInt(vrows);
    cols = parseInt(vcols);
    
    sirikata.event("village_init", position, vcols, vrows);
    gui_grid_init();
}

/* Function: gui_grid_init
 * --------------------------
 * Switches the GUI from the vertex setup mode to the grid-lot setup mode
 */
function gui_grid_init() {
    $("#gui_grid").show();   
    cols = ($('#num_cols').val())-1;
    rows = ($('#num_rows').val())-1;
    space = (cols > rows) ? 400/cols : 400/rows;
    canvas_init();
}

function appendToBody(html) {
    $(html).appendTo('body');
}


/*
 * Main function that gets executed as soon as is ready
 */
$(function() {

    $.getScript("../scripting/villageBuilderHTML.js", function(){
            appendToBody(html);
 

    /* Function: toggleVisible
     * -----------------------
     * toggles the visibility of the gui window
     */
    var toggleVisible = function() {
        window.toggle();    
    };

    VillageBuilder.toggleVisible = toggleVisible;
    
    VillageBuilder.canvas_drawstreet = function(street) {
        sirikata.log("info","\n VillageBuilder.canvas_drawstreet has been called \n");
        sirikata.log("info","\n >> currently "+streets.length+" street edges to draw\n");
        streets.push(street);
        canvas_street_layer_update();
        sirikata.log("info","\n >> now "+streets.length+" street edges to draw\n");
    };

    window = new sirikata.ui.window( 
    "#village-builder",        
       {
           autoOpen: false,
           width: window_width,
           height: window_height,
           modal: false
       }
    );

    $(".facility").draggable({
        helper: "clone",
    });

    $(".template").draggable({
        helper: "clone",
    });

    function canvas_init_arrows(x,y) {
        var size = space/4;

        // creates arrow images to display around facility image
        var n = new Image();
        n.src = arrows['north'];
        var s = new Image();
        s.src = arrows['south'];
        var e = new Image();
        e.src = arrows['east'];
        var w = new Image();
        w.src = arrows['west'];

        n.onload = function() {
            ctx_facility.globalAlpha = 0.9;
            ctx_facility.drawImage(n,x-(size/2),y-(space/2),size,size);
        };

        s.onload = function() {
            ctx_facility.globalAlpha = 0.3;
            ctx_facility.drawImage(s,x-(size/2),y+(space/2)-size,size,size);
        };

        w.onload = function() {
            ctx_facility.globalAlpha = 0.3;
            ctx_facility.drawImage(w,x-(space/2),y-(size/2),size,size);
        };

        e.onload = function() {
            ctx_facility.globalAlpha = 0.3;
            ctx_facility.drawImage(e,x+(space/2)-size,y-(size/2),size,size);
        };

        return {'north':n,'south':s,'east':e,'west':w};
    }
   
    /*
     * Function: canvas_draw_facility
     * ------------------------------
     * Callback function for when a facility icon is dropped onto a canvas
     */
    function canvas_draw_facility(event, ui) {
        var pos = {};
        pos.left = ui.position.left;
        pos.top = ui.position.top + 100; // empirical 100
        pos = convert_to_canvas_coord(pos);

        // creates facility image to display on canvas
        var image = new Image();
        var type = ui.draggable.attr('id');
        image.src = facilityImages[type];
        image.id = facilities.length;
        if(type=="school") var size = space*1.8;
        else var size = space*0.8;

        // col and row to snap to
        var col = Math.floor((pos.x-margin_left)/space);
        var row = Math.floor((pos.y-margin_top)/space);
        if(col < 0 || row < 0 || col > cols-1 || row > rows-1) return;
               
        // x, y positions of gridlines to snap to
        var x = col*space; 
        var y = row*space;
           
        // padding inside a grid lot
        var offset = space*0.1;

        // draw image of facility on canvas
        image.onload = function() {
            ctx_facility.globalAlpha = 1;
            ctx_facility.drawImage(image,margin_left+x+offset,margin_top+y+offset,size,size);
        };

        var arrows = canvas_init_arrows(margin_left+x+space/2, margin_top+y+space/2);
        
        // push type and position to facilities to later display in world
        facilities.push({img:image,type:type,direction:"north",col:col,row:row,arrows:arrows}); 
        
    }


    // Makes the grid layer register objects dragged and dropped in its area
    $("#grid_layer").droppable({
        
        drop: function(event, ui) {
            var tocut = " ui-draggable";
            var cl = ui.draggable.attr('class');
            cl = cl.substring(0,cl.length-tocut.length);
            
            if(cl == "facility") {
                canvas_draw_facility(event,ui);
            } else if(cl == "template") {
                
                // call sirikata.event to include template in villageBuilder_sti
                var type = ui.draggable.attr('id');
                sirikata.event("load_village_street_template",type);

            }
        },
    });

    sirikata.ui.button('#village_init_bttn').button().click(village_init);

    sirikata.ui.button('#village_displaystreets_bttn').button().click(function() {
        sirikata.event("draw_village_streets");
        switch_to_facilities_view();
    });

    sirikata.ui.button('#village_displayfacilities_bttn').button().click(village_displayfacilities);

    sirikata.ui.button('#place_car_bttn').button().click(function() {
        sirikata.event("place_car");
    });

    $('#eraser').click(function() {
        $('#eraser').hide();
        $('#eraser_selected').show();
        erase = true;
    });

    $('#eraser_selected').click(function() {
        $('#eraser_selected').hide();
        $('#eraser').show();
        erase = false;
    });

});

});

});
