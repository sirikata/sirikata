/*
 * File: streetGenerator.em
 * ----------------------
 * 
 *
 */

system.require("std/shim/quaternion.em");
system.import("villageBuilder/streetTools.em");

function createNodeGrid(vertexGrid) {
    var cols = [];
    for(var i = 0; i < vertexGrid.length; i++) {
        var col = [];
        for(var j = 0; j < vertexGrid[i].length; j++) {
            var node = streetNode(vertexGrid[i][j]);
            col.push(node);
        }
        cols.push(col);
    }
    return cols;
}

function blankGrid(nCols, nRows) {
	var cols = [];
	for(var colNum = 0; colNum < nCols; colNum++) {
		var col = [];
		for(var rowNum = 0; rowNum < nRows; rowNum++) {
			col.push(false);
		}
		cols.push(col);
	}
	return cols;
}

function connectRows(vertexGrid, nodeGrid, street) {
    
    var prevNode = null;
    var currNode;
    
    system.print("\nConnecting rows...");
    for(var col = 0; col < vertexGrid.length; col++) {
        for(var row = 0; row < vertexGrid[col].length; row++) {
            currNode = new streetNode(vertexGrid[col][row]);
            nodeGrid[col][row] = currNode;
            street.nodes.push(currNode);
            if(prevNode != null) {
                var edge = new streetEdge(prevNode, currNode);
                
                // update street, edge.nodes, and connected nodes.edges
                street.edges.push(edge);
                edge.nodes.push(prevNode);
                edge.nodes.push(currNode);
                prevNode.edges.push(edge);
                currNode.edges.push(edge);
                
            }
            prevNode = currNode;
        }
        prevNode = null;
    }
}

function connectCols(vertexGrid, nodeGrid, street) {

    var prevNode = null;
    var currNode;
    
    system.print("\nConnecting cols...");
    for(var row = 0; row < nodeGrid[0].length; row++) {
        for(var col = 0; col < nodeGrid.length; col++) {
            currNode = nodeGrid[col][row];
            if(prevNode != null) {
                var edge = new streetEdge(prevNode, currNode);
                
                // update street, edge.nodes and connected nodes. edges
                street.edges.push(edge);
                edge.nodes.push(prevNode);
                edge.nodes.push(currNode);
                prevNode.edges.push(edge);
                currNode.edges.push(edge);
            }
            prevNode = currNode;
        }
        prevNode = null;
    }

}

function extendStreet(street, pos1, pos2) {
    
}

function createEmptyStreet() {
    return {nodes:[],edges:[]};
}

function createStreet(vertexGrid, type) {
    if(type=="grid") {
        var street = {nodes:[],edges:[]};
        var nodeGrid = createNodeGrid(vertexGrid); 
        connectRows(vertexGrid, nodeGrid, street);
        connectCols(vertexGrid, nodeGrid, street);
    } else {
        var street = createPinheadTemplate(vertexGrid);
    }
    return street;
}

/*
 * var createEdge = edgeFactory(path, i);
 * --------------------------------------
 */
function edgeFactory(street, i, scale) {
    var edge = street.edges[i];
    return function(presence) {
        presence.loadMesh(function() {
            var bb = presence.meshBounds().across();
            var x = (edge.nodes[0].position.x + edge.nodes[1].position.x)/2;
            var y = (edge.nodes[0].position.y + edge.nodes[1].position.y + bb.y)/2;
            var z = (edge.nodes[0].position.z + edge.nodes[1].position.z)/2;
            presence.position = <x,y,z>;
            presence.scale = 30*scale;
            presence.orientation = util.Quaternion.fromLookAt(edge.directions[0]);
        });
    }
}

function nodeFactory(street, i, scale) {
    var node = street.nodes[i];
    return function(presence) {
        presence.loadMesh(function() {
            var bb = presence.meshBounds().across();
            presence.position = node.position + <0, bb.y/2, 0>;
            presence.scale = 7.5*scale;
            presence.orientation = util.Quaternion.fromLookAt(node.edges[0].directions[0]);
        });
    }
}


/*
 * function placePath(path)
 * ------------------------
 * Iterates through the edges and renders each edge in the world
 * @param path is the path graph object
 */
function placeStreet(street, space) {
    for(var i = 0; i < street.edges.length; i++) {
        var displayEdge = edgeFactory(street, i, space/60);
        system.createPresence("meerkat:///kittyvision/street.dae/optimized/0/street.dae",displayEdge);
    }
    
    for(var i = 0; i < street.nodes.length; i++) {
        var displayNode = nodeFactory(street, i, space/60);
        system.createPresence("meerkat:///kittyvision/street/intersection.dae/optimized/0/intersection.dae", displayNode);
    }
}
