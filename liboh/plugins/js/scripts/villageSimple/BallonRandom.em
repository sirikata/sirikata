system.require("std/default.em");
system.require("villageSimple/meshes.em");
system.require("std/core/repeatingTimer.em");

meshes = housemeshes.concat(apartmentmeshes).concat(buildingmeshes);

PresQueue = [];

//create a pressence (buiding) at <x,-20,z>
function createPres(mesh,x,z,scale)
{
    system.createEntityScript(<x+(util.rand()-0.5)*3,-20+util.rand()*10,z+(util.rand()-0.5)*3>,
                              newEntExec,
                              null,
                              '0.01',
                              mesh
                             );
}

function newEntExec()
{
    system.require('std/default.em');

    var onPresConnectedCB = function()
    {
        system.self.setScale(2);
        system.self.setOrientation(util.Quaternion.fromLookAt(<0,0,1>));
        system.self.setVelocity(<(util.rand()-0.5)/2, 0, (util.rand()-0.5)/2>);
    };

    system.onPresenceConnected(onPresConnectedCB);

    system.__debugPrint('\nNew entity created\n');

}

function createBuildings(x,z,ini_x,ini_z)
{
    d = 6;
    N = ballonmeshes.length;
    for(var i=0; i<z; i++) {
        for(var j=0; j<x; j++) {
            n = i*z+j
            xx = -(ini_x+j)*d;
            zz = (ini_z+i)*d;
            PresQueue.push([ballonmeshes[n%N],xx,zz,20]);
            //PresQueue.push([cubemesh,xx,zz,20]);
        }
    }
}

function CreatePresCB()
{
    //while(PresQueue.length>0 && windowCount<=windowSize){
    if(PresQueue.length>0) {
        args = PresQueue.shift();
        if(args.length==4) createPres.apply(this,args);
    }
}

//create x by z blocks, with street corner initial position <ini_x*d,-20,ini_z*d>, d is the block size
function createVillage(x,z,ini_x,ini_z)
{
    createBuildings(x,z,ini_x+0.5,ini_z+0.5);
}


function init()
{

    //createPres(terrainmesh,0,0,1000)
    createVillage(5,5,4,-1);

    var repTimer = new std.core.RepeatingTimer(1,CreatePresCB);
}

