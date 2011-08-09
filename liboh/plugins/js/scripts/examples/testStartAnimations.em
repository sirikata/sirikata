// Two ways of discovering and starting animations from Emerson.


//////////////////  METHOD 1  ////////////////////////////

function meshLoaded(pres) {
  system.print(pres + " : meshLoaded\n");
  
  var animationList = pres.getAnimationList();
  
  if (animationList.length > 0)
    simulator.startAnimation(pres,animationList[0]);
  
}

var visibleMap = system.getProxSet(system.self);
for (var key in visibleMap) {
  visibleMap[key].loadMesh( std.core.bind(meshLoaded, undefined, visibleMap[key]) );
}
  
  
  
//////////////////////// METHOD 2 ///////////////////////////
  
  
var visibleMap = system.getProxSet(system.self);
for (var key in visibleMap) {
  animationList = simulator.getAnimationList(visibleMap[key]);
  
  if (animationList.length > 0)
    simulator.startAnimation(visibleMap[key], animationList[0]);
}
