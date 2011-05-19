

system.require('std/core/queryDistance.em');


function addFunc()
{
    system.print('\n\nWithin range\n\n');
}

function removeFunc()
{
    system.print('\n\nOut of range\n\n');
}

var qD = new std.core.QueryDistance(3,addFunc,removeFunc,system.self);




