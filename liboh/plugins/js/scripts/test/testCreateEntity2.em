

function mFunc(arg)
{
    system.import('std/default.em');
    
    for (var s in arg)
        system.print(s.toString() + ':' + arg[s].toString());

    var onpresCon = function()
    {
        system.print('\n\nI got a presence\n\n');
        system.print(system.self.toString());
        system.print(system.self.getPosition());
        system.self.setScale(4);
        system.print('\n\n');
    };
    system.onPresenceConnected(onpresCon);
    
    system.print('\n\nnooo\n\n');
}


function mFunc_new(arg)
{
    system.import('std/default.em');
    system.print('\n\nooo\n\n');
}



var a={
    'a':'a',
    'b': 'b',
    'c':'c',
    'd': "something\""
};

//a = 'aaaaaa';

var newMesh  = 'meerkat:///benchristel/brokenwall.dae/original/0/brokenwall.dae';

//var whichVar = system.createEntity2(system.self.getPosition() + <2,0,0>,mFunc,a,newMesh,system.self.getScale(),55);


var stringToExec = '('+mFunc.toString() + ')(null);';

var whichVar = system.createEntity2(system.self.getPosition() + <2,0,0>,stringToExec,null,newMesh,system.self.getScale(),55);



