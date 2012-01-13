system.require("std/default.em");
system.require("villageSimple/meshes.em");
system.require("std/core/repeatingTimer.em");

meshes = housemeshes.concat(apartmentmeshes).concat(buildingmeshes);

PresQueue = [];
PresCount = 1;
windowSize = 10;
windowCount = 0;

//create a pressence (buiding) at <x,-20,z>
function createPres(mesh,x,z,scale)
{
    system.createEntityScript(<x,-20,z>,
                              newEntExec,
                              null,
                              3,
                              mesh
                             );
}

function newEntExec()
{
    system.require('std/default.em');

    var onPresConnectedCB = function()
    {
        system.self.setScale(5);
    };

    system.onPresenceConnected(onPresConnectedCB);

    system.__debugPrint('\nGot into new entity\n');

}

function createBuildings(x,z,ini_x,ini_z)
{
    d = 10;
    N = meshes.length;
    for(var i=0; i<z; i++) {
        for(var j=0; j<x; j++) {
            n = i*z+j
            xx = -(ini_x+j)*d;
            zz = (ini_z+i)*d;
            //PresQueue.push([meshes[n%N],xx,zz,20]);
            PresQueue.push([cubemesh,xx,zz,20]);
        }
    }
}

function createStreets(x,z,ini_x,ini_z)
{
    d = 50;
    for(var i=0; i<z; i++) {
        for(var j=0; j<x; j++) {
            xx = -(ini_x+j)*d;
            zz = (ini_z+i)*d;
            xxx= -(ini_x+j+1)*d;
            zzz= (ini_z+i+1)*d;
            //if(j!=x-1) PresQueue.push([streetmesh, xx, zz, xxx, zz]);
            //if(i!=z-1) PresQueue.push([streetmesh, xx, zz, xx, zzz]);
            if(j!=x-1) PresQueue.push([cubemesh, xx, zz, xxx, zz]);
            if(i!=z-1) PresQueue.push([cubemesh, xx, zz, xx, zzz]);
        }
    }
}

function CreatePresCB()
{
    //while(PresQueue.length>0 && windowCount<=windowSize){
    if(PresQueue.length>0) {
        windowCount++;
        //system.print(windowCount);
        //system.print('----current windowCount\n');
        args = PresQueue.shift();
        if(args.length==4) createPres.apply(this,args);
        //else if(args.length==5) createStreet.apply(this,args);
    }
}

//create x by z blocks, with street corner initial position <ini_x*d,-20,ini_z*d>, d is the block size
function createVillage(x,z,ini_x,ini_z)
{
    createBuildings(x,z,ini_x+0.5,ini_z+0.5);
    //createStreets(x+1,z+1,ini_x,ini_z);
}


function init()
{

    //createPres(terrainmesh,0,0,1000)
    createVillage(5,5,0,-1);

    var repTimer = new std.core.RepeatingTimer(1,CreatePresCB);
/*
    system.onPresenceConnected(function()
                               {
                                   windowCount--;
                                   //system.print(windowCount);
                                   //system.print('----current windowCount\n');
                                   PresCount++;
                                   system.print(PresCount);
                                   system.print('----presenced connected\n');

                                   CreatePresCB();
                               }
                              );

    CreatePresCB();


    system.onPresenceDisconnected(function()
                                  {
                                      PresCount--;
                                      system.print(PresCount);
                                      system.print('----Presence Disconnceted\n');
                                  }
                                 );
*/
}

