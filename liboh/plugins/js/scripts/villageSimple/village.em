system.import("villageSimple/meshes.em");

meshes = housemeshes.concat(apartmentmeshes).concat(buildingmeshes);

function createBuilding(x,z,scale)
{
    //system.print("New presence is connected!");
    return function(presence) {
        presence.setScale(scale);
        presence.loadMesh(function() {
            var bb = presence.meshBounds().across();
            var pos = <x, bb.y/2-20, z>;
            presence.position = pos;
            presence.orientation = util.Quaternion.fromLookAt(<0,0,1>);
        });
    }

}

function createBuildings(x,z)
{
    d = 50;
    N = meshes.length;
    ini_x = 0.5;
    ini_z = -(z-1)/2;
    for(var i=0; i<z; i++) {
        for(var j=0; j<x; j++) {
            n = i*z+j
            xx = -(ini_x+j)*d;
            zz = (ini_z+i)*d;
            system.createPresence(meshes[n%N],createBuilding(xx,zz,20));
        }
    }
}


//create a street from <ax,0,az> to <bx,0,bz>
function createStreet(ax,az,bx,bz)
{
    d=50;
    return function(presence) {
        presence.loadMesh(function() {
            if (ax==bx)
                presence.orientation = util.Quaternion.fromLookAt(<0,0,1>);             
            else if (az==bz)
                presence.orientation = util.Quaternion.fromLookAt(<1,0,0>);
            var bb = presence.meshBounds().across();
            system.print(bb);
            var pos = <(ax+bx)/2, bb.y/2-20, (az+bz)/2>;
            presence.position = pos;
            if (ax==bx) presence.setScale(d/bb.z);            
            else if (az==bz) presence.setScale(d/bb.x);
        });
    }

}


function createIntersection(x,z)
{
    return function(presence) {
        presence.loadMesh(function() {
            presence.orientation = util.Quaternion.fromLookAt(<1,0,0>);
            var bb = presence.meshBounds().across();
            var pos = <x, bb.y/2-20, z>;
            presence.position = pos;
            presence.setScale(5);
        });
    }
}


function createStreets(x,z)
{
    ini_x = 0;
    ini_z = -(z-1)/2;
    for(var i=0; i<z; i++) {
        for(var j=0; j<x; j++) {
            xx = -(ini_x+j)*d;
            zz = (ini_z+i)*d;
            xxx= -(ini_x+j+1)*d;
            zzz= (ini_z+i+1)*d; 
            system.createPresence(intersectionmesh, createIntersection(xx,zz));
            if(j!=x-1) system.createPresence(streetmesh, createStreet(xx,zz,xxx,zz));
            if(i!=z-1) system.createPresence(streetmesh, createStreet(xx,zz,xx,zzz));
        }
    }

}

function createVillage(x,z)
{
    createBuildings(x,z);
    createStreets(x+1,z+1);
}

createVillage(3,4)