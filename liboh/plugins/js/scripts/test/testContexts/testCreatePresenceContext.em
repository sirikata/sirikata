//checks whether can create a new presence from the context
//then prints out the length of the presences vector both before and
//after creating the new context to ensure that context was registered
//with correct vector.  From scripting terminal, you should also check
//length of the presences vector to ensure that only the internal
//context's presence vector grows.


system.import('test/testContexts/baseContextTest.em');


function testContext()
{
    var myMesh = system.presences[0].getMesh();
    newContext.execute(toExecute,myMesh);
}


function toExecute(meshName)
{
    var onConnectedFunc = function()
    {
      system.print("\n\nI am connected\n\n");  
    };
    
    system.print("\n\nInside of toExecute\n");
    system.print("\n\n\n");

    system.print("\nLength of pres vector before: ");
    system.print(system.presences.length);
    system.print('\n');
    
    system.create_presence(meshName,onConnectedFunc);


    system.print("\nLength of pres vector after: ");
    system.print(system.presences.length);
    system.print('\n');
    
}
    
testContext();



