system.import("villageBuilder/islandGenerator.em");
system.require("std/shim/bbox.em");
system.require("std/shim/raytrace.em");

var vertexGrid = []; //global

function createVertexGrid(islandPos, cols, rows, space, callback) {
    system.__debugPrint("In createVertexGrid!\n");
    system.__debugPrint(" >> cols: "+cols +" rows: "+rows+" space: "+space);
    function constructVertexGrid() {
        var offset = space*cols*multiplier/4;
        var x = islandPos.x - islandBB.x/2 + offset*multiplier;
        var y = islandPos.y + islandBB.y/2;
        var z = islandPos.z - islandBB.z/2 + offset*0.8;
        var anchor = <x,y,z>;
        
        //var vertexGrid = [];
        for(var i = 0; i < cols; i++) {
            var col = [];
            for(var j = 0; j < rows; j++) {
                var pos = <anchor.x+space*i,anchor.y,anchor.z+space*j>;
                col.push(pos);
            }
            vertexGrid.push(col);
        }
        callback(vertexGrid, space);
    }

    var multiplier = 5/3;
    //var islandPos = <0,0,0>;
    var islandBB = <0,0,0>;
    createIsland(space*cols*multiplier, islandPos, islandBB, constructVertexGrid);
}

system.import("villageBuilder/streetGenerator-standalone.em");
