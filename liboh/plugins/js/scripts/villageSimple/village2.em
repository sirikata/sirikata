system.require("villageSimple/meshes.em");
system.require("std/core/repeatingTimer.em");
system.require("villageSimple/map.em");

meshes = housemeshes.concat(apartmentmeshes).concat(buildingmeshes);

PresQueue = [];

//create a pressence (buiding) at <x,-20,z>
function createPres(mesh,x,z,scale)
{
    system.createPresence({'mesh': mesh, 
                           'pos': <x,0,z>,
                           'orient': util.Quaternion.fromLookAt(<0,0,1>),
                           'scale': scale,
                           'callback': function (presence) {
                               presence.loadMesh(function() {
                                   bb = presence.meshBounds().across();
                                   pos = <x, bb.y/2-20, z>;
                                   presence.position = pos;
                               });
                           }
                          }
                         );
}

//create a street from <ax,-20,az> to <bx,-20,bz>
function createStreet(mesh,ax,az,bx,bz)
{
    d = 100;
    scale = 1;
    system.createPresence({'mesh': mesh,
                           'pos': <(ax+bx)/2,0,(az+bz)/2>, 
                           'callback': function(presence) {
                               presence.loadMesh(function() {
                                   if (ax==bx)
                                       presence.orientation = util.Quaternion.fromLookAt(<0,0,1>);
                                   else if (az==bz)
                                       presence.orientation = util.Quaternion.fromLookAt(<1,0,0>);
                                   bb = presence.meshBounds().across();
                                   pos = <(ax+bx)/2, bb.y/2-20, (az+bz)/2>;
                                   presence.position = pos;
                                   if (ax==bx) scale=d/bb.z;
                                   else if (az==bz) scale=(d+d*bb.z/bb.x)/bb.x;
                                   presence.setScale(scale);
                               });
                           }
                          }
                         );
}

function createBuildings(ini_x,ini_z)
{
    d = 100;
    NR = housemeshes.length;
    NC = apartmentmeshes.length;
    nr = 0;
    nc = 0;
    for(i=1; i<villageMap.length; i=i+2) {
        for(j=1; j<villageMap[i].length; j=j+2) {
            xx = -(ini_x+(j-1)/2)*d;
            zz = (ini_z+(i-1)/2)*d;
            if(villageMap[i][j]=='R'){
                nr++;
                PresQueue.push([housemeshes[nr%NR],xx+d/4,zz+d/4,20]);
                nr++;
                PresQueue.push([housemeshes[nr%NR],xx+d/4,zz-d/4,20]);
                nr++;
                PresQueue.push([housemeshes[nr%NR],xx-d/4,zz+d/4,20]);
                nr++;
                PresQueue.push([housemeshes[nr%NR],xx-d/4,zz-d/4,20]);
            }
            if(villageMap[i][j]=='C'){
                nc++;
                PresQueue.push([apartmentmeshes[nc%NC],xx,zz,50]);
            }
        }
    }
}


function createStreets(ini_x,ini_z)
{
    d = 100;
    for(var i=0; i<villageMap.length; i=i+2) {
        for(var j=1; j<villageMap[i].length; j=j+2) {
            xx = -(ini_x+(j-1)/2)*d;
            zz = (ini_z+i/2)*d;
            if(villageMap[i][j]=='-') 
                PresQueue.push([streetmesh, xx, zz, xx-d, zz]);
        }
    }
    for(var i=1; i<villageMap.length; i=i+2) {
        for(var j=0; j<villageMap[i].length; j=j+2) {
            xx = -(ini_x+j/2)*d;
            zz = (ini_z+(i-1)/2)*d;
            if(villageMap[i][j]=='|')
                PresQueue.push([streetmesh, xx, zz, xx, zz+d]);
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

//create x by z blocks, with street corner initial position <ini_x*d,-20,ini_z*d>, d is the block size
function createVillage()
{
    createBuildings(initial_x+0.5, initial_z+0.5);
    createStreets(initial_x,initial_z);
}


function init()
{
    //createPres(terrainmesh,0,0,1000)
    createVillage();
    system.onPresenceConnected(CreatePresCB);
    CreatePresCB();
}

