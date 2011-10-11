system.import("villageBuilder/treemeshes.em");
system.import("villageBuilder/flowermeshes.em");
system.import("villageBuilder/rockmeshes.em");

var zombielib = [];
zombielib.push("meerkat:///kittyvision/zombie4.dae/optimized/0/zombie4.dae");
zombielib.push("meerkat:///kittyvision/zombie3.dae/optimized/0/zombie3.dae");
zombielib.push("meerkat:///kittyvision/zombie2.dae/optimized/0/zombie2.dae");
zombielib.push("meerkat:///kittyvision/zombie1.dae/optimized/0/zombie1.dae");
zombielib.push("meerkat:///kittyvision/blood.dae/optimized/0/blood.dae");

var aptlib = [];
//aptlib.push({mesh:"meerkat:///kittyvision/apartment5.dae/optimized/apartment5.dae",
//             modelOrientation: -0.5*Math.PI});
aptlib.push({mesh:"meerkat:///kittyvision/apartment2.dae/optimized/apartment2.dae",
             modelOrientation: Math.PI});
aptlib.push({mesh:"meerkat:///kittyvision/apartment1.dae/optimized/apartment1.dae",
             modelOrientation: Math.PI});
/*
var patiochairlib = [];
patiochairlib.push({mesh:"
*/
// fencelib
var fencelib = [];
fencelib.push({mesh:"meerkat:///kittyvision/fence_white_big.dae/optimized/fence_white_big.dae",
               modelOrientation:0});
fencelib.push({mesh:"meerkat:///kittyvision/fence_spider.dae/optimized/fence_spider.dae",
               modelOrientation:0});

var houselib = [];
houselib.push({mesh:"meerkat:///emily2e/models/SabbathDayHouse.dae/optimized/0/SabbathDayHouse.dae",
               modelOrientation: 0.75*Math.PI});
houselib.push({mesh:"meerkat:///emily2e/models/esteramoise.dae/optimized/0/esteramoise.dae",
               modelOrientation: 0.5*Math.PI});
houselib.push({mesh:"meerkat:///emily2e/models/redhouse.dae/optimized/0/redhouse.dae",
               modelOrientation: 0.5*Math.PI});
houselib.push({mesh:"meerkat:///kittyvision/house1.dae/optimized/0/house1.dae",
               modelOrientation: 0});
houselib.push({mesh:"meerkat:///kittyvision/house2.dae/optimized/0/house2.dae",
               modelOrientation: Math.PI});
houselib.push({mesh:"meerkat:///kittyvision/house4.dae/optimized/0/house4.dae",
               modelOrientation: Math.PI});
houselib.push({mesh:"meerkat:///kittyvision/house16.dae/original/0/house16.dae",
               modelOrientation: 1.2*Math.PI});
/*
houselib.push({mesh:"meerkat:///kittyvision/house21.dae/optimized/house21.dae",
               modelOrienetation: Math.PI});
*/

var facilitymeshes = [];

var schoolmeshes = [];
// school building
schoolmeshes.push({mesh:"meerkat:///kittyvision/highschool.dae/optimized/highschool.dae",
                   offset:<-0.3,0,0.1>, scale:0.275, rotation: 0, modelOrientation:Math.PI});
// playground
schoolmeshes.push({mesh:"meerkat:///kittyvision/playground.dae/optimized/0/playground.dae",
                   offset:<0.4,0,0.35>, scale:0.1, rotation: 1.5*Math.PI, modelOrientation:0});
// schoolbus
schoolmeshes.push({mesh:"meerkat:///kittyvision/schoolbus2.dae/original/0/schoolbus2.dae",
                   offset:<-0.48,0,0.33>, scale:0.04, rotation: Math.PI, modelOrientation:Math.PI});
schoolmeshes.push({mesh:"meerkat:///kittyvision/schoolbus2.dae/original/0/schoolbus2.dae",
                   offset:<-0.48,0,0.47>, scale:0.04, rotation: Math.PI, modelOrientation:Math.PI});

// parking lot
schoolmeshes.push({mesh: "meerkat:///kittyvision/parking.dae/optimized/parking.dae",
                offset: <-0.35,0,0.4>, scale: 0.085, rotation: 1.5*Math.PI, modelOrientation: 0});
schoolmeshes.push({mesh: "meerkat:///kittyvision/parking.dae/optimized/parking.dae",
                offset: <-0.3,0,0.4>, scale: 0.085, rotation: 0.5*Math.PI, modelOrientation: 0});
schoolmeshes.push({mesh: "meerkat:///kittyvision/parking.dae/optimized/parking.dae",
                offset: <-0.15,0,0.4>, scale: 0.085, rotation: 1.5*Math.PI, modelOrientation: 0});
schoolmeshes.push({mesh: "meerkat:///kittyvision/parking.dae/optimized/parking.dae",
                offset: <-0.1,0,0.4>, scale: 0.085, rotation: 0.5*Math.PI, modelOrientation: 0});

// picnic tables and trees
for(var x = 0.1; x < 0.5; x += 0.14) {
    schoolmeshes.push({mesh:"meerkat:///kittyvision/picnic_table.dae/optimized/picnic_table.dae",
                       offset:<x-0.06,0,0>, scale:0.02, rotation: 0.5*Math.PI, modelOrientation: 0});
    schoolmeshes.push({mesh:treemeshes[Math.floor(Math.random()*treemeshes.length)],
                       offset: <x+0.03,0,-0.06>, scale: 0.04, rotation: 0, modelOrientation: 0});
    schoolmeshes.push({mesh:"meerkat:///kittyvision/picnic_table.dae/optimized/picnic_table.dae",
                       offset:<x,0,0>, scale:0.02, rotation: 0.5*Math.PI, modelOrientation: 0});
// the following code is commented out because it tends to make the world crash
/* 
    schoolmeshes.push({mesh:"meerkat:///kittyvision/picnic_table.dae/optimized/picnic_table.dae",
                       offset:<x-0.06,0,0.1>, scale:0.02, rotation: 0.5*Math.PI, modelOrientation: 0});
    schoolmeshes.push({mesh:treemeshes[Math.floor(Math.random()*treemeshes.length)],
                       offset: <x+0.03,0,0.096>, scale: 0.04, rotation: 0, modelOrientation: 0});
    schoolmeshes.push({mesh:"meerkat:///kittyvision/picnic_table.dae/optimized/picnic_table.dae",
                       offset:<x,0,0.1>, scale:0.02, rotation: 0.5*Math.PI, modelOrientation: 0});
*/}

// athletic facilities
schoolmeshes.push({mesh:"meerkat:///kittyvision/track.dae/optimized/track.dae",
                   offset:<-0.3,0,-0.35>,scale:0.3,rotation:0.5*Math.PI,modelOrientation:0});
schoolmeshes.push({mesh:"meerkat:///kittyvision/schoolgym.dae/optimized/schoolgym.dae",
                   offset:<0.32,0,-0.3>,scale:0.15,rotation:0, modelOrientation:0.16*Math.PI});

var firestationmeshes = [];
firestationmeshes.push({mesh:"meerkat:///kittyvision/firestation.dae/optimized/0/firestation.dae",
                   offset:<0,0,-0.2>, scale:0.6, rotation: 0, modelOrientation: 0.5*Math.PI});
firestationmeshes.push({mesh:"meerkat:///kittyvision/firetruck2.dae/original/0/firetruck2.dae",
                   offset:<-0.2,0,0.35>, scale:0.18, rotation: 0.5*Math.PI, modelOrientation:Math.PI});


var zombiemeshes = [];
zombiemeshes.push({mesh:zombielib[Math.floor(Math.random()*zombielib.length)],
                   offset:<0.25,0,0>, scale:0.2, rotation: 0, modelOrientation: Math.random()*2*Math.PI});
zombiemeshes.push({mesh:zombielib[Math.floor(Math.random()*zombielib.length)],
                   offset:<0.2,0,-0.15>, scale:0.2, rotation: 0, modelOrientation: Math.random()*2*Math.PI});
zombiemeshes.push({mesh:zombielib[Math.floor(Math.random()*zombielib.length)],
                   offset:<0.4,0,-0.4>, scale:0.2, rotation: 0, modelOrientation: Math.random()*2*Math.PI});
zombiemeshes.push({mesh:zombielib[Math.floor(Math.random()*zombielib.length)],
                   offset:<0.35,0,-0.25>, scale:0.2, rotation: 0, modelOrientation: Math.random()*2*Math.PI});
zombiemeshes.push({mesh:zombielib[Math.floor(Math.random()*zombielib.length)],
                   offset:<0.35,0,0.1>, scale:0.2, rotation: 0, modelOrientation: Math.random()*2*Math.PI});
zombiemeshes.push({mesh:zombielib[Math.floor(Math.random()*zombielib.length)],
                   offset:<0.45,0,-0.6>, scale:0.2, rotation: 0, modelOrientation: Math.random()*2*Math.PI});
zombiemeshes.push({mesh:zombielib[Math.floor(Math.random()*zombielib.length)],
                   offset:<-0.65,0,-0.1>, scale:0.2, rotation: 0, modelOrientation: Math.random()*2*Math.PI});
zombiemeshes.push({mesh:zombielib[Math.floor(Math.random()*zombielib.length)],
                   offset:<0.5,0,0.6>, scale:0.2, rotation: 0, modelOrientation: Math.random()*2*Math.PI});

var housemeshes = [];
var index = Math.floor(Math.random()*houselib.length);
housemeshes.push({mesh: houselib[index].mesh, offset:<0.2,0,0.15>, scale:0.183,
                   rotation: 0, modelOrientation: houselib[index].modelOrientation});
index = Math.floor(Math.random()*houselib.length);
housemeshes.push({mesh: houselib[index].mesh, offset:<-0.3,0,0.15>, scale:0.183,
                   rotation:0, modelOrientation: houselib[index].modelOrientation});
index = Math.floor(Math.random()*houselib.length);
housemeshes.push({mesh: houselib[index].mesh, offset:<-0.2,0,-0.15>, scale:0.183,
                   rotation: Math.PI, modelOrientation: houselib[index].modelOrientation});
index = Math.floor(Math.random()*houselib.length);
housemeshes.push({mesh: houselib[index].mesh, offset:<0.3,0,-0.15>, scale:0.183,
                   rotation: Math.PI, modelOrientation: houselib[index].modelOrientation});
index = Math.floor(Math.random()*fencelib.length);

// vertical row of fences
housemeshes.push({mesh: fencelib[index].mesh, offset:<0,0,0.05>, scale:0.05,
                    rotation: 0.5*Math.PI, modelOrientation: fencelib[index].modelOrientation});
housemeshes.push({mesh: fencelib[index].mesh, offset:<0,0,0.14>, scale:0.05,
                    rotation: 0.5*Math.PI, modelOrientation: fencelib[index].modelOrientation});
housemeshes.push({mesh: fencelib[index].mesh, offset:<0,0,0.23>, scale:0.05,
                    rotation: 0.5*Math.PI, modelOrientation: fencelib[index].modelOrientation});
housemeshes.push({mesh: fencelib[index].mesh, offset:<0,0,0.32>, scale:0.05,
                    rotation: 0.5*Math.PI, modelOrientation: fencelib[index].modelOrientation});
housemeshes.push({mesh: fencelib[index].mesh, offset:<0,0,0.41>, scale:0.05,
                    rotation: 0, modelOrientation: fencelib[index].modelOrientation});

housemeshes.push({mesh: fencelib[index].mesh, offset:<0,0,-0.05>, scale:0.05,
                    rotation: 0.5*Math.PI, modelOrientation: fencelib[index].modelOrientation});
housemeshes.push({mesh: fencelib[index].mesh, offset:<0,0,-0.14>, scale:0.05,
                    rotation: 0.5*Math.PI, modelOrientation: fencelib[index].modelOrientation});
housemeshes.push({mesh: fencelib[index].mesh, offset:<0,0,-0.23>, scale:0.05,
                    rotation: 0.5*Math.PI, modelOrientation: fencelib[index].modelOrientation});
housemeshes.push({mesh: fencelib[index].mesh, offset:<0,0,-0.32>, scale:0.05,
                    rotation: 0.5*Math.PI, modelOrientation: fencelib[index].modelOrientation});
housemeshes.push({mesh: fencelib[index].mesh, offset:<0,0,-0.41>, scale:0.05,
                    rotation: 0, modelOrientation: fencelib[index].modelOrientation});

// horizontal row of fences
housemeshes.push({mesh: fencelib[index].mesh, offset:<0.05,0,0>, scale:0.05,
                    rotation: 0, modelOrientation: fencelib[index].modelOrientation});
housemeshes.push({mesh: fencelib[index].mesh, offset:<0.14,0,0>, scale:0.05,
                    rotation: 0, modelOrientation: fencelib[index].modelOrientation});
housemeshes.push({mesh: fencelib[index].mesh, offset:<0.23,0,0>, scale:0.05,
                    rotation: 0, modelOrientation: fencelib[index].modelOrientation});
housemeshes.push({mesh: fencelib[index].mesh, offset:<0.32,0,0>, scale:0.05,
                    rotation: 0, modelOrientation: fencelib[index].modelOrientation});
housemeshes.push({mesh: fencelib[index].mesh, offset:<0.41,0,0>, scale:0.05,
                    rotation: 0, modelOrientation: fencelib[index].modelOrientation});

housemeshes.push({mesh: fencelib[index].mesh, offset:<-0.05,0,0>, scale:0.05,
                    rotation: 0, modelOrientation: fencelib[index].modelOrientation});
housemeshes.push({mesh: fencelib[index].mesh, offset:<-0.14,0,0>, scale:0.05,
                    rotation: 0, modelOrientation: fencelib[index].modelOrientation});
housemeshes.push({mesh: fencelib[index].mesh, offset:<-0.23,0,0>, scale:0.05,
                    rotation: 0, modelOrientation: fencelib[index].modelOrientation});
housemeshes.push({mesh: fencelib[index].mesh, offset:<-0.32,0,0>, scale:0.05,
                    rotation: 0, modelOrientation: fencelib[index].modelOrientation});
housemeshes.push({mesh: fencelib[index].mesh, offset:<-0.41,0,0>, scale:0.05,
                    rotation: 0, modelOrientation: fencelib[index].modelOrientation});

// peripheral fences
housemeshes.push({mesh: fencelib[index].mesh, offset:<0.48,0,0.05>, scale:0.05,
                    rotation: 0.5*Math.PI, modelOrientation: fencelib[index].modelOrientation});
housemeshes.push({mesh: fencelib[index].mesh, offset:<0.48,0,0.14>, scale:0.05,
                    rotation: 0.5*Math.PI, modelOrientation: fencelib[index].modelOrientation});
housemeshes.push({mesh: fencelib[index].mesh, offset:<0.48,0,0.23>, scale:0.05,
                    rotation: 0.5*Math.PI, modelOrientation: fencelib[index].modelOrientation});
housemeshes.push({mesh: fencelib[index].mesh, offset:<0.48,0,0.32>, scale:0.05,
                    rotation: 0.5*Math.PI, modelOrientation: fencelib[index].modelOrientation});
housemeshes.push({mesh: fencelib[index].mesh, offset:<0.48,0,0.41>, scale:0.05,
                    rotation: 0, modelOrientation: fencelib[index].modelOrientation});

housemeshes.push({mesh: fencelib[index].mesh, offset:<0.48,0,-0.05>, scale:0.05,
                    rotation: 0.5*Math.PI, modelOrientation: fencelib[index].modelOrientation});
housemeshes.push({mesh: fencelib[index].mesh, offset:<0.48,0,-0.14>, scale:0.05,
                    rotation: 0.5*Math.PI, modelOrientation: fencelib[index].modelOrientation});
housemeshes.push({mesh: fencelib[index].mesh, offset:<0.48,0,-0.23>, scale:0.05,
                    rotation: 0.5*Math.PI, modelOrientation: fencelib[index].modelOrientation});
housemeshes.push({mesh: fencelib[index].mesh, offset:<0.48,0,-0.32>, scale:0.05,
                    rotation: 0.5*Math.PI, modelOrientation: fencelib[index].modelOrientation});
housemeshes.push({mesh: fencelib[index].mesh, offset:<0.48,0,-0.41>, scale:0.05,
                    rotation: 0, modelOrientation: fencelib[index].modelOrientation});


housemeshes.push({mesh: fencelib[index].mesh, offset:<-0.48,0,0.05>, scale:0.05,
                    rotation: 0.5*Math.PI, modelOrientation: fencelib[index].modelOrientation});
housemeshes.push({mesh: fencelib[index].mesh, offset:<-0.48,0,0.14>, scale:0.05,
                    rotation: 0.5*Math.PI, modelOrientation: fencelib[index].modelOrientation});
housemeshes.push({mesh: fencelib[index].mesh, offset:<-0.48,0,0.23>, scale:0.05,
                    rotation: 0.5*Math.PI, modelOrientation: fencelib[index].modelOrientation});
housemeshes.push({mesh: fencelib[index].mesh, offset:<-0.48,0,0.32>, scale:0.05,
                    rotation: 0.5*Math.PI, modelOrientation: fencelib[index].modelOrientation});
housemeshes.push({mesh: fencelib[index].mesh, offset:<-0.48,0,0.41>, scale:0.05,
                    rotation: 0, modelOrientation: fencelib[index].modelOrientation});

housemeshes.push({mesh: fencelib[index].mesh, offset:<-0.48,0,-0.05>, scale:0.05,
                    rotation: 0.5*Math.PI, modelOrientation: fencelib[index].modelOrientation});
housemeshes.push({mesh: fencelib[index].mesh, offset:<-0.48,0,-0.14>, scale:0.05,
                    rotation: 0.5*Math.PI, modelOrientation: fencelib[index].modelOrientation});
housemeshes.push({mesh: fencelib[index].mesh, offset:<-0.48,0,-0.23>, scale:0.05,
                    rotation: 0.5*Math.PI, modelOrientation: fencelib[index].modelOrientation});
housemeshes.push({mesh: fencelib[index].mesh, offset:<-0.48,0,-0.32>, scale:0.05,
                    rotation: 0.5*Math.PI, modelOrientation: fencelib[index].modelOrientation});
housemeshes.push({mesh: fencelib[index].mesh, offset:<-0.48,0,-0.41>, scale:0.05,
                    rotation: 0, modelOrientation: fencelib[index].modelOrientation});

// flowers
/*housemeshes.push({mesh: flowermeshes[Math.floor(Math.random()*flowermeshes.length)],
                  offset:<0.2,0,0.4>, scale:0.01, rotation: 0, modelOrientation: 0});
housemeshes.push({mesh: flowermeshes[Math.floor(Math.random()*flowermeshes.length)],
                  offset:<0.15,0,0.37>, scale:0.01, rotation: 0, modelOrientation: 0});
housemeshes.push({mesh: flowermeshes[Math.floor(Math.random()*flowermeshes.length)],
                  offset:<0.23,0,0.42>, scale:0.01, rotation: 0, modelOrientation: 0});
*/
// trees
housemeshes.push({mesh: treemeshes[Math.floor(Math.random()*treemeshes.length)],
                  offset:<0.45,0,-0.35>, scale:0.13,rotation:0,modelOrientation:0});
housemeshes.push({mesh: treemeshes[Math.floor(Math.random()*treemeshes.length)],
                  offset:<-0.05,0,-0.35>, scale:0.13,rotation:0,modelOrientation:0});
housemeshes.push({mesh: treemeshes[Math.floor(Math.random()*treemeshes.length)],
                  offset:<0.05,0,0.35>, scale:0.13,rotation:0,modelOrientation:0});
housemeshes.push({mesh: treemeshes[Math.floor(Math.random()*treemeshes.length)],
                  offset:<-0.45,0,0.35>, scale:0.13,rotation:0,modelOrientation:0});


var parkmeshes = [];
// rocks for path
var index = Math.floor(Math.random()*rockmeshes.length);
parkmeshes.push({mesh: rockmeshes[index], offset:<-0.4,0,0.35>, scale: 0.05,
                 rotation: Math.random()*2*Math.PI, modelOrientation: 0});
index = Math.floor(Math.random()*rockmeshes.length);
parkmeshes.push({mesh: rockmeshes[index], offset:<-0.3,0,0.25>, scale: 0.05,
                 rotation: Math.random()*2*Math.PI, modelOrientation: 0});
index = Math.floor(Math.random()*rockmeshes.length);
parkmeshes.push({mesh: rockmeshes[index], offset:<-0.2,0,0.2>, scale: 0.05,
                 rotation: Math.random()*2*Math.PI, modelOrientation: 0});
index = Math.floor(Math.random()*rockmeshes.length);
parkmeshes.push({mesh: rockmeshes[index], offset:<-0.1,0,0.1>, scale: 0.05,
                 rotation: Math.random()*2*Math.PI, modelOrientation: 0});
index = Math.floor(Math.random()*rockmeshes.length);
parkmeshes.push({mesh: rockmeshes[index], offset:<0,0,0>, scale: 0.05,
                 rotation: Math.random()*2*Math.PI, modelOrientation: 0});
index = Math.floor(Math.random()*rockmeshes.length);
parkmeshes.push({mesh: rockmeshes[index], offset:<0.1,0,-0.15>, scale: 0.05,
                 rotation: Math.random()*2*Math.PI, modelOrientation: 0});
index = Math.floor(Math.random()*rockmeshes.length);
parkmeshes.push({mesh: rockmeshes[index], offset:<0.2,0,-0.3>, scale: 0.05,
                 rotation: Math.random()*2*Math.PI, modelOrientation: 0});
index = Math.floor(Math.random()*rockmeshes.length);
parkmeshes.push({mesh: rockmeshes[index], offset:<0.3,0,-0.4>, scale: 0.05,
                 rotation: Math.random()*2*Math.PI, modelOrientation: 0});
// trees
index = Math.floor(Math.random()*treemeshes.length);
parkmeshes.push({mesh: treemeshes[index], offset:<0.1,0,0.4>, scale: 0.17,
                 rotation: Math.random()*2*Math.PI, modelOrientation: 0});
index = Math.floor(Math.random()*treemeshes.length);
parkmeshes.push({mesh: treemeshes[index], offset:<0.2,0,0.15>, scale: 0.17,
                 rotation: Math.random()*2*Math.PI, modelOrientation: 0});
index = Math.floor(Math.random()*treemeshes.length);
parkmeshes.push({mesh: treemeshes[index], offset:<0.35,0,0.38>, scale: 0.17,
                 rotation: Math.random()*2*Math.PI, modelOrientation: 0});
index = Math.floor(Math.random()*treemeshes.length);
parkmeshes.push({mesh: treemeshes[index], offset:<0.4,0,-0.2>, scale: 0.17,
                 rotation: Math.random()*2*Math.PI, modelOrientation: 0});
index = Math.floor(Math.random()*treemeshes.length);
parkmeshes.push({mesh: treemeshes[index], offset:<0.35,0,-0.32>, scale: 0.17,
                 rotation: Math.random()*2*Math.PI, modelOrientation: 0});
index = Math.floor(Math.random()*treemeshes.length);
parkmeshes.push({mesh: treemeshes[index], offset:<-0.25,0,0.04>, scale: 0.17,
                 rotation: Math.random()*2*Math.PI, modelOrientation: 0});
index = Math.floor(Math.random()*treemeshes.length);
parkmeshes.push({mesh: treemeshes[index], offset:<-0.35,0,0.15>, scale: 0.17,
                 rotation: Math.random()*2*Math.PI, modelOrientation: 0});
index = Math.floor(Math.random()*treemeshes.length);
parkmeshes.push({mesh: treemeshes[index], offset:<-0.3,0,-0.2>, scale: 0.17,
                 rotation: Math.random()*2*Math.PI, modelOrientation: 0});
index = Math.floor(Math.random()*treemeshes.length);
parkmeshes.push({mesh: treemeshes[index], offset:<-0.32,0,-0.3>, scale: 0.17,
                 rotation: Math.random()*2*Math.PI, modelOrientation: 0});

// flowers
index = Math.floor(Math.random()*flowermeshes.length);
parkmeshes.push({mesh: flowermeshes[index], offset:<0.1,0,0.27>, scale: 0.08,
                 rotation: Math.random()*2*Math.PI, modelOrientation: 0});
index = Math.floor(Math.random()*flowermeshes.length);
parkmeshes.push({mesh: flowermeshes[index], offset:<0.35,0,0.06>, scale: 0.08,
                 rotation: Math.random()*2*Math.PI, modelOrientation: 0});
index = Math.floor(Math.random()*flowermeshes.length);
parkmeshes.push({mesh: flowermeshes[index], offset:<-0.3,0,-0.2>, scale: 0.08,
                 rotation: Math.random()*2*Math.PI, modelOrientation: 0});
index = Math.floor(Math.random()*flowermeshes.length);
parkmeshes.push({mesh: flowermeshes[index], offset:<-0.4,0,-0.3>, scale: 0.08,
                 rotation: Math.random()*2*Math.PI, modelOrientation: 0});
index = Math.floor(Math.random()*flowermeshes.length);
parkmeshes.push({mesh: flowermeshes[index], offset:<-0.2,0,-0.3>, scale: 0.08,
                 rotation: Math.random()*2*Math.PI, modelOrientation: 0});
index = Math.floor(Math.random()*flowermeshes.length);
parkmeshes.push({mesh: flowermeshes[index], offset:<-0.1,0,-0.4>, scale: 0.08,
                 rotation: Math.random()*2*Math.PI, modelOrientation: 0});
index = Math.floor(Math.random()*flowermeshes.length);
parkmeshes.push({mesh: flowermeshes[index], offset:<-0.15,0,-0.37>, scale: 0.08,
                 rotation: Math.random()*2*Math.PI, modelOrientation: 0});
index = Math.floor(Math.random()*flowermeshes.length);
parkmeshes.push({mesh: flowermeshes[index], offset:<-0.25,0,-0.32>, scale: 0.08,
                 rotation: Math.random()*2*Math.PI, modelOrientation: 0});
index = Math.floor(Math.random()*flowermeshes.length);
parkmeshes.push({mesh: flowermeshes[index], offset:<0.16,0,0.14>, scale: 0.08,
                 rotation: Math.random()*2*Math.PI, modelOrientation: 0});
index = Math.floor(Math.random()*flowermeshes.length);
parkmeshes.push({mesh: flowermeshes[index], offset:<0.2,0,0.17>, scale: 0.08,
                 rotation: Math.random()*2*Math.PI, modelOrientation: 0});
// sign
parkmeshes.push({mesh: "meerkat:///kittyvision/southparksign.dae/optimized/0/southparksign.dae",
                 offset:<-0.4,0,0.4>, scale: 0.11, rotation: 0, modelOrientation: 0});

// little market
parkmeshes.push({mesh: "meerkat:///danielrh/market.dae/optimized/0/market.dae",
                 offset:<-0.05,0,-0.3>, scale: 0.115, rotation: -1.13*Math.PI, modelOrientation: Math.PI});


var aptmeshes = [];
var index = Math.floor(Math.random()*aptlib.length);
// apt bulidings
aptmeshes.push({mesh: aptlib[index].mesh,
                offset: <0,0,-0.3>, scale:0.35, rotation: 0, modelOrientation: aptlib[index].modelOrientation}); 
aptmeshes.push({mesh: aptlib[index].mesh,
                offset: <0.4,0,0.09>, scale:0.35, rotation: 0.5*Math.PI, modelOrientation: aptlib[index].modelOrientation}); 

// parking spaces
aptmeshes.push({mesh: "meerkat:///kittyvision/parking.dae/optimized/parking.dae",
                offset: <-0.33,0,-0.25>, scale: 0.17, rotation: 0.5*Math.PI, modelOrientation: 0});
aptmeshes.push({mesh: "meerkat:///kittyvision/parking.dae/optimized/parking.dae",
                offset: <-0.43,0,-0.25>, scale: 0.17, rotation: 1.5*Math.PI, modelOrientation: 0});
aptmeshes.push({mesh: "meerkat:///kittyvision/parking.dae/optimized/parking.dae",
                offset: <0.33,0,0.38>, scale: 0.17, rotation: 0, modelOrientation: 0});
aptmeshes.push({mesh: "meerkat:///kittyvision/parking.dae/optimized/parking.dae",
                offset: <0.33,0,0.45>, scale: 0.17, rotation: Math.PI, modelOrientation: 0});

// outdoor pool
aptmeshes.push({mesh: "meerkat:///kittyvision/pool1.dae/optimized/pool1.dae",
                offset: <0.35,0,-0.275>, scale: 0.1, rotation: 0, modelOrientation: 0});




facilitymeshes["school"] = schoolmeshes;
facilitymeshes["apartment"] = aptmeshes;
facilitymeshes["firestation"] = firestationmeshes;
facilitymeshes["zombies"] = zombiemeshes;
facilitymeshes["house"] = housemeshes;
facilitymeshes["park"] = parkmeshes;
