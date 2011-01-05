

system.print("\n\nTesting whether the stillVisible command works\n\n");


var allCalledBack = new Array();


function proxCallback(calledBack)
{
    if (calledBack !== undefined)
    {
        system.print("\n\nGot a proximity callback.");
        allCalledBack.push(calledBack);
        system.print("\nAbout to check the calledBack object\n");
        system.print(allCalledBack[allCalledBack.length-1].stillVisible.toString() + "\n");
        system.print("Done checking the calledBack object\n");
    }
}

system.presences[0].onProxAdded(proxCallback);

//the timeoutCallback prints which objects are stillVisible and which ones are not.  By moving up to the objects and back again, we should be able to test whether the stillVisible attribute actually works.

function timeoutCallback()
{
    system.print("\n\nThis timeoutcallback:\n\n");
    
    var numStillVis = 0;
    var numVis = 0;
    for (var s=0; s < allCalledBack.length; ++s)
    {
        system.print("\n" + s.toString() + " of " + allCalledBack.length.toString()+"\n");
        if (allCalledBack[s].stillVisible)
        {
            ++numStillVis;                
        }
        else
        {
            ++numVis;                
        }
    }

    system.print("\n\nThis is total:");
    system.print((numVis + numStillVis).toString());
    system.print("\nThis is numStillVis: " + numStillVis.toString());
    system.print("\n\nThis is numVis:  " + numVis.toString());
    system.print("\n\n");
    system.timeout(2,null,timeoutCallback);
}



system.presences[0].setQueryAngle(.4);

system.timeout(2,null,timeoutCallback);