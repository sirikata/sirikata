

system.print("\n\nSCRIPT TO TEST PROX EVENTS\n\n");


//this script tests the proximity-exposed code in emerson: it first registers handlers on presence_0 for
//if the object

numInProx = 0;

function proxAddedCallback(arg)
{
    numInProx +=1;
    system.print("\n\nProx added callback completed\n");
    system.print(numInProx);
    system.print("\n\n");
}

function proxRemovedCallback(arg)
{
    numInProx -=1;
    system.print("\n\nProx removed callback completed\n");
    system.print(numInProx);
    system.print("\n\n");
}

system.presences[0].onProxAdded(proxAddedCallback);
system.presences[0].onProxRemoved(proxRemovedCallback);


system.presences[0].setQueryAngle(.1);