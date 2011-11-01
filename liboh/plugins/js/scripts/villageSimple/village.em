system.import("villageSimple/meshes.em");

meshes = housemeshes.concat(apartmentmeshes).concat(buildingmeshes);

PresQueue = [];

function createPres(mesh,x,z,scale)
{
    system.createPresence(mesh, 
                          function (presence) {
                              presence.setScale(scale);
                              presence.loadMesh(function() {
                                  var bb = presence.meshBounds().across();
                                  var pos = <x, bb.y/2-20, z>;
                                  presence.position = pos;
                                  presence.orientation = util.Quaternion.fromLookAt(<0,0,1>);
                              });
                          }
                         );
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
            PresQueue.push([meshes[n%N],xx,zz,20]);
        }
    }
}

//create a street from <ax,0,az> to <bx,0,bz>                                                                                                                
function createStreet(mesh,ax,az,bx,bz)
{
    d=50;
    system.createPresence(mesh, 
                          function(presence) {
                              presence.loadMesh(function() {
                                  if (ax==bx)
                                      presence.orientation = util.Quaternion.fromLookAt(<0,0,1>);
                                  else if (az==bz)
                                      presence.orientation = util.Quaternion.fromLookAt(<1,0,0>);
                                  var bb = presence.meshBounds().across();
                                  var pos = <(ax+bx)/2, bb.y/2-20, (az+bz)/2>;
                                  presence.position = pos;
                                  if (ax==bx) presence.setScale(d/bb.z);
                                  else if (az==bz) presence.setScale(d/bb.x);
                              });
                          }
                         );
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
            PresQueue.push([intersectionmesh, xx, zz, 5]);
            if(j!=x-1) PresQueue.push([streetmesh, xx, zz, xxx, zz]);
            if(i!=z-1) PresQueue.push([streetmesh, xx, zz, xx, zzz]);
        }
    }

}

function CreatePresCB()
{
    if (PresQueue.length>0) {
        args = PresQueue.shift();
        if(args.length==4) createPres.apply(this,args);
        else if(args.length==5) createStreet.apply(this,args);
    }
}

function createVillage(x,z)
{
    createBuildings(x,z);
    createStreets(x+1,z+1);
}
repTimer = std.core.RepeatingTimer(1,CreatePresCB);

createVillage(3,3);

