system.print("\n\nFollowing test will take my current position and create another object next to me that looks the same as me.\n\n");

system.import('test/testContexts/baseContextTest.em');


var newPos = system.presences[0].getPosition();
newPos.x = newPos.x + 2;


function testContext()
{
    newContext.execute(toExecute,newPos);
}


function toExecute(newPos)
{
    system.create_entity(newPos,"js","test/testPrint.em","meerkat:///danielrh/Insect-Bird.dae",1.0,1);
    system.print("\n\nCreating a new entity\n\n");
    system.print("\n\n");
}

testContext();



