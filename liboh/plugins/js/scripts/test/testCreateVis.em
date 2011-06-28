

//this file is run on entity A.  It creates two entities, B and C.
//When B is created, it sends a message back to A.
//That message contains the allData object from calling getAllData on
//its own presence.  A sends that allData object to
//C, who then calls system.createVisible with it.  If everything
//functions correctly, should be able to send a message
//from C to B.  B has a function handler set up to receive that
//message, and start moving in the world when it gets it.



var cVis = null;
var bSelfData = null;
function bMessageHandler(msg,sender)
{
    bSelfData = msg;
    if (cVis != null)
    {
        sendCData(msg);
    }
        
}


function sendCData(msgToSend)
{
    msgToSend >> cVis >>[];
}



bMessageHandler << {'bField':: };


function cMessageHandler(msg,sender)
{
    cVis = sender;
    if (bSelfData != null)
        sendCData(bSelfData);
        
}

cMessageHandler << { 'cField' ::};

function cInitializer(myMaster)
{
    system.require('std/default.em');

    
    var masterVis = system.createVisible(myMaster.masterAddr);
    var presConFunc = function()
    {
        
        var processMasterMessage = function(msg,sender)
        {
            //var otherVis = system.createVisible(msg.sporef, msg.sporefFrom, msg.pos,msg.vel, msg.posTime, msg.orient, msg.orientVel, msg.orientTime,msg.pos, msg.scale, msg.mesh);
            var otherVis = system.createVisible(msg.sporef,
                                                msg.pos,
                                                msg.vel,
                                                msg.posTime,
                                                msg.orient,
                                                msg.orientVel,
                                                msg.orientTime,
                                                msg.pos,
                                                msg.scale,
                                                msg.mesh,
                                                msg.physics);

            system.print('\n\nThis is the position of the object that I am sending to: ');
            system.print( msg.pos);
            system.print(msg.pos.x);
            system.print(',');
            system.print(msg.pos.y);
            system.print(',');
            system.print(msg.pos.z);
            system.print('\n\n');

            msg >> otherVis >>[];
        };

        processMasterMessage << {'sporef'::} << masterVis;

        
        var msg = {'cField': "c's_registration"};
        msg >> masterVis >>[];
    };


    system.onPresenceConnected(presConFunc);
}



function bInitializer(myMaster)
{
    system.require('std/default.em');
    var masterVis = system.createVisible(myMaster.masterAddr);

    
    var presConFunc  = function()
    {
        //handler registration
        var onMessageReceipt = function(msg, sender)
        {
            system.print('\nI got a message to set velocity for.\n');
            system.self.setVelocity(<1,0,0>);
        };

        
        onMessageReceipt << {'sporef'::};

                
        system.print('\n\nIn b.  This is my position: '  + system.self.getPosition().toString());

        
        var allSelfData = system.self.toVisible().getAllData();
        allSelfData.bField = "b's_registration";
        allSelfData>>masterVis>>[];
        
    };
    
    system.onPresenceConnected(presConFunc);

}



var masterArg =  {
  'masterAddr'  : system.self.toVisible().toString()
};

var meshB  = 'meerkat:///palmfreak/torus_knot.dae/original/0/torus_knot.dae';
var meshC  = 'meerkat:///palmfreak/totoro.dae/original/0/totoro.dae';



system.createEntityScript(system.self.getPosition() + <3,0,0>, bInitializer,masterArg, 55, meshB);
system.createEntityScript(system.self.getPosition() + <-3,0,0>, cInitializer,masterArg, 55, meshC);

