system.import("std/shim/bbox.em");
system.import("std/shim/raytrace.em");

function createIsland(size, islandPos, islandBB, callback) {
    function setIslandPosition(message, sender) {
        system.__debugPrint("\nHave received message that island has been created!");
        island.scale = size;
        island.loadMesh(function() {
            bb = island.meshBounds().across();
            islandBB.x = bb.x;
            islandBB.y = bb.y;
            islandBB.z = bb.z;
            
            
            island.position = islandPos + <0, -bb.y/2, 0>;
            islandPos.x = island.position.x;
            islandPos.y = island.position.y;
            islandPos.z = island.position.z;
            
            {islandIsReady:true} >> system.self >> [];
        });
    }

    var island;
    system.createPresence("meerkat:///kittyvision/floatingland.dae/optimized/0/floatingland.dae",function(presence) {
        system.__debugPrint("\nCreating island...");
        island = presence;
        {islandCreated:true} >> system.self >> [];
    });
    
    setIslandPosition << [{'islandCreated'::}];
    callback << [{'islandIsReady'::}];
}
