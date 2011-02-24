

var myMesh = system.presences[0].getMesh();

function newPresMove(presToMove)
{
    system.print("\n\nGot into the newPresMove function!\n\n");
    presToMove.setVelocity(new util.Vec3(-.1,0,0));
}



function newPresCallback()
{
}


var newPres = system.create_presence(myMesh,newPresCallback);


var predicate = function()
{
    return newPres.isConnected;
};


var toDo = function ()
{
    newPresMove(newPres);
};

var whener = util.create_when(
    predicate,
    toDo,
    10,
    newPres);

