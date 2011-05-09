
var visArray = new Array();
system.self.onProxAdded(proxAddedFunc);
system.self.setQueryAngle(55);
system.self.setQueryAngle(.00001);

function proxAddedFunc(newVis)
{
    visArray.push(newVis);
}

system.import('std/core/repeatingTimer.em');

var mTimer = std.core.RepeatingTimer(6, tCB);

function tCB()
{
    var toBroadcast = {
        "msg": "msg",
        "m": "random"
    };

    for (var s in visArray)
    {
        var mNewVis = system.createVisible(visArray[s].toString());
        toBroadcast->mNewVis;
        
    }
    
}



var mVisible = system.createVisible(system.self.toVisible().toString());
//var mVisible = __system.createVisible(system.self.toVisible().toString());