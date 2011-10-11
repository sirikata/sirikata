/*
 * File: carGenerator.em
 * created by G1
 * ---------------------
 * Given a graph object of a street system, lets a car loose on the road.
 * 
 * Usage:
 *      system.import(<streetGenerator.em file here>);
 *      var street = createStreet(<appropriate parameters here>);
 *      system.import(<carGenerator.em file here>);
 *      for(var i = 0; i < <# cars to be created>; i++) createCar(street);
 *
 */

system.require("std/shim/quaternion.em");
system.require("std/movement/units.em");
system.require("villageBuilder/carmeshes.em");

/*
 * When called, stops the rotation of the presence passed in as a parameter
 */
function stopRotation(presence) {
    presence.orientationVel = new util.Quaternion();
}

/*
 * This function lets the presence passed in as a parameter gradually face
 * its target direction by rotating at a specified rate instead of jumping
 * to face the target direction.
 *
 * @param presence The presence that is to rotate
 * @param oldDirectionVec The direction vector for the presence's current
 *			orientation
 * @param targetDirectionVec The direction vector for the presences's target
 *			orientation
 * @param rotationPeriod The total amount of time it will take for the
 * 			presence to make a 360-degree rotation
 */
function rotate(presence, oldDirectionVec, targetDirectionVec, rotationPeriod) {
    var cross = targetDirectionVec.cross(oldDirectionVec);
    var theta = Math.acos( targetDirectionVec.dot(oldDirectionVec)/(targetDirectionVec.length()*oldDirectionVec.length()) );
    var time = theta/(2*Math.PI/rotationPeriod);
    if(cross.y > 0) { // clockwise
        presence.orientationVel = (new util.Quaternion(<0,-1,0>,1)).scale(2*Math.PI/rotationPeriod);
    } else { // counter-clockwise
        presence.orientationVel = (new util.Quaternion(<0,1,0>,1)).scale(2*Math.PI/rotationPeriod);
    }
    system.timeout(time, function() {
        stopRotation(presence);
    });
}

/*
 * Given the current edge the car is traveling through and the index of the node
 * the car is traveling towards, selects the next edge and node for the car.
 */
function selectNextEdge(index, edge) {

    // If there is only one edge connected to the node the car is traveling to,
    // then it means that the road is not through. So by returning the current
    // edge as the next edge, the car takes a u-turn.
    if(edge.nodes[index].edges.length == 1) {
        return edge;   
    }
    
    // Else, creates an array of edges radiating from the target node that does not
    // include the current node, so that a new edge can be randomly selected.
    var viableEdges = [];
    for(var i = 0; i < edge.nodes[index].edges.length; i++) {
        if(edge.nodes[index].edges[i] == edge) continue;
        viableEdges.push(edge.nodes[index].edges[i]);
    }
    
    // If there is more than one edge connected to the node, but there are no viable
    // edges other than the current edge, that means the the only other edge is blocked.
    // Thus car makes a u-turn.
    if(viableEdges.length == 0) return edge;
    
    // Otherwise, selects randomly from the viable edges array
    else return viableEdges[Math.floor(Math.random()*viableEdges.length)];
}

/*
 * Given the current target node and the next edge the car will be traveling on,
 * returns the next target node by checking in with the current target node and
 * making sure that it isn't the one that is being returned.
 */
function selectNextIndex(index, currEdge, nextEdge) {
    if(nextEdge.nodes[0] == currEdge.nodes[index]) return 1;
    return 0;
}

/*
 * Given two nodes, returns the vector resulting from subtracting the position of
 * the second node from that of the first.
 */
function diffVec(nextNode, prevNode) {
    var x = nextNode.position.x - prevNode.position.x;
    var y = nextNode.position.y - prevNode.position.y;
    var z = nextNode.position.z - prevNode.position.z;
    var diff = <x,y,z>;
    return diff;
}

/*
 * Returns the direction the car will turn in at the next intersection
 */
function getTurn(currDir, nextDir) {
    var theta = Math.acos(nextDir.dot(currDir)/(nextDir.length()*currDir.length()));
    
    if(theta < 0.1*Math.PI) return "straight";
    if(theta > 0.9*Math.PI) return "u";

    var cross = nextDir.cross(currDir);
    if(cross.y > 0) { // clockwise
        return "right";
    }
    return "left";

}

/*
 * Given the target node it is driving towards, this function sets the correct
 * velocity and orientation for the car. Then the next target node to travel
 * towards is selected and a recursive call is made so that the car would orient
 * itself correctly after it has finished traveling to the current target node.
 */
function recurseDrive(index, edge) {
    var currDirectionVec = system.self.velocity;
    
    var multiplier = 0.5;
    var dx = edge.nodes[index].position.x - system.self.position.x;
    var dy = 0;
    var dz = edge.nodes[index].position.z - system.self.position.z;
    var diff = <dx, dy, dz>;
    var vel = <dx*multiplier, dy*multiplier, dz*multiplier>;
    system.self.velocity = <0,0,0>;
    
    var nextEdge = selectNextEdge(index, edge);
    var nextIndex = selectNextIndex(index, edge, nextEdge);
 
    var rotationPeriod;
    var turn = getTurn(currDirectionVec, vel);
    if(turn=="straight") {
        rotationPeriod = 0;
    } else {
        if(turn!="u") rotationPeriod = 0.5;
        else rotationPeriod = 1;
        rotate(system.self, currDirectionVec, vel, rotationPeriod);
    }
    
    system.timeout(rotationPeriod*0.5, function() {
        system.self.velocity = vel;
        system.timeout(diff.length()/vel.length(), function() {
            recurseDrive(nextIndex, nextEdge);
        });
    });
}

/*
 * Given a graph representation of a system of streets to travel on, this function
 * creates a car and lets it loose on the road. A random mesh for the car is picked
 * from the carmeshes array.
 */
function createCar(graph, space){
    if(graph.nodes.length == 0 || graph.edges.length == 0) {
        system.__debugPrint("\nError: street is an empty or malformed graph");
        return;
    }
    var index = Math.floor(Math.random()*carmeshes.length);
    system.createPresence(carmeshes[index].mesh, function(presence) {
        presence.loadMesh(function() {
            var bb = presence.untransformedMeshBounds().across();
            presence.scale = space*0.075;
            presence.position = <graph.nodes[0].position.x, graph.nodes[0].position.y+(bb.y*space*0.075)/2, graph.nodes[0].position.z>;
           
            system.__debugPrint("\nRotating by "+carmeshes[index].modelOrientation+" radians");
            var tweak = new util.Quaternion(<0,1,0>, carmeshes[index].modelOrientation);
            var dir = tweak.mul(graph.nodes[0].edges[0].directions[1]);
            presence.orientation = util.Quaternion.fromLookAt(dir);
            presence.velocity = graph.nodes[0].edges[0].directions[1].scale(0.1);
            system.timeout(3,function() {
                recurseDrive(1,graph.edges[0]);
            });
        });
    });
}
