

system.print("\n\nSCRIPT TO TEST PROX EVENTS\n\n");


//this script tests the proximity-exposed code in emerson: it first registers handlers on presence_0 for
//if the object

numInProx = 0;

system.import('test/testContexts/baseContextTest.em');

function testContext()
{
    var mesher = system.presences[0].getMesh();
    newContext.execute(toExecute,mesher);
}



function toExecute(myMesh)
{
    numInProx = 0;

    
    var onConnected = function(presConnection)
    {
        var proxAddedCallback = function(arg)
        {
            numInProx +=1;
            system.print("\n\nProx added callback completed\n");
            system.print(numInProx);
            system.print("\n\n");
            presConnection.setVelocity(new util.Vec3(1,0,0));
            presConnection.setVelocity(new util.Vec3(1,0,0));

        };

        var proxRemovedCallback = function(arg)
        {
            numInProx -=1;
            system.print("\n\nProx removed callback completed\n");
            system.print(numInProx);
            
            system.print("\n\n");
            //presConnection.setVelocity(new util.Vec3(0,0,0));
        };
    
        presConnection.onProxAdded(proxAddedCallback);
        presConnection.onProxRemoved(proxRemovedCallback);

        presConnection.setQueryAngle(.4);
        system.print("\n\nNew presence is connected.  Now, I'm doing the prox added and removed stuff.\n\n");
        presConnection.setVelocity(new util.Vec3(1,0,0));
    };
    
    //first create a new presence
    system.create_presence(myMesh,onConnected);

}

testContext();

