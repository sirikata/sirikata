

system.print("\n\nFollowing test will take my current position and create another object next to me that looks the same as me.\n\n");

var newPos = system.presences[0].getPosition();
newPos.x = newPos.x + 2;

system.create_entity(newPos,"js","test/testPrint.em","meerkat:///danielrh/Insect-Bird.dae",1.0,1);

