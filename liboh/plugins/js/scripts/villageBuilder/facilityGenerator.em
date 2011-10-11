system.import("villageBuilder/facilitymeshes.em");


function meshFactory(obj, direction, anchor, space) {
    return function(presence) {
        presence.loadMesh(function() {
            var bb = presence.meshBounds().across();
            var pos = anchor + <space/2, bb.y*space*obj.scale/2+0.05, space/2>;
            var ahead = direction.scale(space*obj.offset.z);
            var right = direction.scale(space*obj.offset.x);
            var up = direction.scale(space*obj.offset.y);        
    
            // Vector 90 degrees clockwise to edge.directions[dir] around the y axis, scaled according to offset
            var rotation = new util.Quaternion(<0,1,0>, -0.5*Math.PI);
            right = rotation.mul(right);
            
            // Vector 90 degrees counter-clockwise to edge.directions[dir] around vector z, scaled.
            rotation = new util.Quaternion(right,0.5*Math.PI);
            up = rotation.mul(up);
            
            var x = pos.x + ahead.x + right.x + up.x;
            var y = pos.y + ahead.y + right.y + up.y;
            var z = pos.z + ahead.z + right.z + up.z;
            var finalPos = <x,y,z>;
            
            presence.position = finalPos;
            
            var tweak = new util.Quaternion(<0,1,0>, obj.rotation + obj.modelOrientation);
            var dir = tweak.mul(direction);
            presence.orientation = util.Quaternion.fromLookAt(dir);

            presence.scale = space*obj.scale;
        });
    }
}

function translateDirection(direction) {
    var dir;
    if(direction == "north") {
        dir = <0,0,-1>;
    } else if(direction == "east") {
        dir = <1,0,0>;
    } else if(direction == "south") {
        dir = <0,0,1>;
    } else {
        dir = <-1,0,0>;
    }
    return dir;
}

function createFacility(type, direction, anchor, space) {
    system.__debugPrint("\nIN createFacility of type "+type+"\n");
    var dir = translateDirection(direction);
    var meshes = facilitymeshes[type];
    system.__debugPrint("\n >> meshes.length is "+meshes.length+"\n");
    for(var i = 0; i < meshes.length; i++) {
        var createMesh = meshFactory(meshes[i], dir, anchor, space);
        system.createPresence(meshes[i].mesh, createMesh);
    }
}

