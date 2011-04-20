function constructCallback(num, pos) {
  return function() {
    system.print("Callback on presence " + num);
    //system.presences[num].setPosition(pos);
  };
};

for (var i = 0; i < 5; i++) {
  var pos = system.presences[0].getPosition();
  pos.x += 5;
  system.print("Constructing presence " + i);
  system.create_presence("meerkat:///test/sphere.dae/original/0/sphere.dae", constructCallback(i, pos));
    
}
