/*
 * File: streetTools.em
 * -------------------
 */

/*
 * var node = new pathNode(<0,0,0>);
 * ---------------------------------
 * Constructor for a path node.
 * @param position of the node
 */
function streetNode(position) {
    this.position = position;
    this.edges = [];
}

/*
 * var edge = new pathEdge(startNode, endNode);
 * --------------------------------------------
 * Constructor for a path edge.
 * @param startNode
 * @param endNode
 */
function streetEdge(startNode, endNode) {
    this.rendered = false;
    this.nodes = [startNode, endNode];
    this.types = [];
    
    this.directions = [];
    this.directions.push(startNode.position - endNode.position);
    this.directions.push(endNode.position - startNode.position);
}

function isSamePos(pos1, pos2) {
    if(pos1.x==pos2.x && pos1.y==pos2.y && pos1.z==pos2.z) return true;
    return false;
}

function findEdge(street, pos1, pos2) {
    for(var i = 0; i < street.edges.length; i++) {
        var edge = street.edges[i];

        
        if(isSamePos(pos1,edge.nodes[0].position) &&
           isSamePos(pos2,edge.nodes[1].position)) {
                return {edge:edge, index:i};
        }
    }
    return null;
}

function findNode(street, pos) {
    for(var i = 0; i < street.nodes.length; i++) {
        var node = street.nodes[i];
        if(isSamePos(pos,node.position)) {
            return {node:node,index:i};
        }
    }
    return null;
}

function addNode(street, pos) {
    var found = findNode(street, pos);
    if(found == null) {
        found = new streetNode(pos);
        street.nodes.push(found);
    } else {
        found = found.node;
    }
    return found;
}

function deleteNode(street, node) {
    var found = findNode(street, node.position);
    street.nodes.splice(found.index,1);
}

function deleteEdge(street, edge, node) {
    if(node.edges.length == 1) {
        deleteNode(street, node);
    }
    for(var i = 0; i < node.edges.length; i++) {
        var pos1 = node.edges[i].nodes[0].position;
        var pos2 = node.edges[i].nodes[1].position;
        if((isSamePos(pos1,edge.nodes[0].position) &&
           isSamePos(pos2,edge.nodes[1].position)) ||
           (isSamePos(pos1,edge.nodes[1].position) &&
           isSamePos(pos2,edge.nodes[0].position))) {
            node.edges.splice(i,1);
        }
    }
}

function eraseStreet(street, pos1, pos2) {
    var edgeObj = findEdge(street, pos1, pos2); 
    if(edgeObj == null) return;
    var edge = edgeObj.edge;
    var index = edgeObj.index;
    
    deleteEdge(street, edge, edge.nodes[0]);
    deleteEdge(street, edge, edge.nodes[1]);
    street.edges.splice(index,1);
}

function extendStreet(street, pos1, pos2) {
   if( findEdge(street, pos1, pos2) != null) return false; // edge already exists, so no need to extend street 
   var start = addNode(street, pos1);
   var finish = addNode(street, pos2);
   var edge = new streetEdge(start, finish);
   
   street.edges.push(edge);
   start.edges.push(edge);
   finish.edges.push(edge);
   return true;
}

function createEmptyStreet() {
    return {nodes:[],edges:[]};
}


function pushNextStreet(st, prev, pos) {
    var next = new streetNode(pos);
    st.nodes.push(next);
    var edge = new streetEdge(prev, next);
    st.edges.push(edge);
    prev.edges.push(edge);
    next.edges.push(edge);
    return next;
}

function createPinheadTemplate(street, vertexGrid) {
    system.__debugPrint("\nCreating pinhead!");
    var streets = [];

    if(vertexGrid.length < 5 || vertexGrid[0].length < 5) {
        system.__debugPrint("\nError: vertexGrid is not big enough for pinhead template");
        return street;
    }
   
    var centercol = Math.floor(vertexGrid.length/2);
    var centerrow = Math.floor(vertexGrid[0].length/2);
    var quarter = Math.floor(vertexGrid.length/4);
    var lastrow = vertexGrid[0].length - 1;
    for(var r = 0; r < centerrow; r++) {
        var flag = extendStreet(street, vertexGrid[centercol][r], vertexGrid[centercol][r+1]);
        if(flag) streets.push({c1:centercol,r1:r,c2:centercol,r2:r+1});
    }

    for(var c = centercol-quarter; c < centercol+quarter; c++) {
        var flag = extendStreet(street, vertexGrid[c][centerrow], vertexGrid[c+1][centerrow]);
        if(flag) streets.push({c1:c,r1:centerrow,c2:c+1,r2:centerrow});
        
        flag = extendStreet(street, vertexGrid[c][lastrow], vertexGrid[c+1][lastrow]);
        if(flag) streets.push({c1:c,r1:lastrow,c2:c+1,r2:lastrow});
    }
   
    for(var r = centerrow; r < lastrow; r++) {
        var flag = extendStreet(street, vertexGrid[centercol-quarter][r], vertexGrid[centercol-quarter][r+1]);
        if(flag) streets.push({c1:centercol-quarter,r1:r,c2:centercol-quarter,r2:r+1});

        flag = extendStreet(street, vertexGrid[centercol+quarter][r], vertexGrid[centercol+quarter][r+1]);
        if(flag) streets.push({c1:centercol+quarter,r1:r,c2:centercol+quarter,r2:r+1});
    }

    return streets;
}

function createTemplate(st, template,vertexGrid) {
    if(template=="pinhead") return createPinheadTemplate(st,vertexGrid);
}
