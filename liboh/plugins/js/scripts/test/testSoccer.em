

//have a ball and soccer players that are playing in a circular area.
//If the ball ever leaves the circular area, the captain runs to get it.
//None of the players ever leave the circular area.  Instead, they run around it.
//If any of the players are near a ball, they reserve the right to kick it.  Then, they run towards it, and the ball zooms off in  the direction that the player kicked it.


function presAdded()
{
    system.print("\n\npres added\n\n");
}


function degreesToRadians(degree)
{
    return 3.14159*degree/180.0;
}

function getMaxAngle(playerPos,centerPos,dist)
{
    return getAngle(playerPos,centerPos,dist) + degreesToRadians(15.0);
}

function getBaseAngle(playerPos,centerPos,dist)
{
    return getAngle(playerPos,centerPos,dist) + degreesToRadians(-15.0);
}


function getAngle(from, to,dist)
{
    return util.asin( (to.y - from.y) /dist);
}


//takes in a presence, angle that you want the player to be traveling to relative to the 
function setVelocity(player,angle,speed)
{
    var xSpeed = util.cos(angle)*speed;
    var ySpeed = util.sin(angle)*speed;
    var zSpeed = 0;
    var newVeloc = new util.Vec3(xSpeed,ySpeed,zSpeed);
    player.setVelocity(newVeloc);
}



function createTeamObject(ball)
{
    var team = util.create_watched();
    team.count = 0;
    team.maxPlayers = 11;
    team.kickLock = false;
    
    team.playerMesh = "meerkat:///danielrh/Poisonbird.dae";
    team.ball = ball;
    team.center = ball.getPosition();
    team.radius = 30;
    team.playerSpeed = .5;
    team.ballSpeed = 1;
    team.allPlayers = new Array();
    team.captain = null;
    team.addPlayer = function(newPlayer)
    {
        team.allPlayers.push(newPlayer);
        if (team.count == 0)
        {
            team.captain = newPlayer;
        }
        team.count = team.count + 1;
    };

    team.playerWanderPeriod = 4;
    team.playerWander = function (playerToWander)
    {
        //if moving out of center of circle
        var angle = 0;
        var baseAngle = 0;
        var maxAngle = degreesToRadians(360);
        var dist = playerToWander.distance(team.center);        
        if (dist > team.radius)
        {
            baseAngle = getBaseAngle(playerToWander.getPosition(),team.center,dist);
            maxAngle  = getMaxAngle(playerToWander.getPosition(),team.center,dist);
        }
        
        angle = baseAngle + (maxAngle - baseAngle)*util.rand();
        setVelocity(playerToWander,angle,team.playerSpeed);
        system.timeout(team.playerWanderPeriod,null,team.playerWander);
    };

    team.kickBall = function(playerToKick)
    {
        team.kickLock = true;
        playerToKick.setPosition(team.ball.getPosition());
        var angle = util.rand()*degreesToRadians(360);
        setVelocity(team.ball,angle,team.ballSpeed);
        team.kickLock = false;
    };

    team.fetchBall = function(playerToFetch)
    {
        team.kickLock = true;
        playerToFetch.setPosition(team.ball.getPosition());
        var dist = team.ball.distance(team.center);
        var angle = getAngle(team.ball.getPosition(),team.center,dist);
        setVelocity(team.ball,angle,team.ballSpeed);
    };

    return team;
}

mTeam = createTeamObject(system.presences[0]); 

for (var s=0; s < mTeam.maxPlayers; ++s)
{
    var newPres = system.create_presence(mTeam.playerMesh,presAdded);

    var predicate = function()
    {
        return newPres.isConnected;
    };


    var callbacker = function()
    {
        mTeam.addPlayer(newPres);
    };
    
    var whener = util.create_when(
        predicate,
        callbacker,
        5,
        newPres);
}



var predTeamBegin = function()
{
    return (mTeam.count == mTeam.maxPlayers);
};
var cbTeamBegin = function()
{
    system.print("\n\ngot into cbTeamBegin\n\n");
    playSoccer(mTeam);
};

util.create_when(
    predTeamBegin,
    cbTeamBegin,
    5,
    mTeam);



function playSoccer(team)
{
    system.print("\n\ngot into the playsoccer function\n\n");

    for (var s=0; s < team.allPlayers.length; ++s)
    {
        //system.print("\n\nprinting\n\n");
        team.playerWander(team.allPlayers[s]);            
    }
}


function playSoccer2(team)
{
    //when ball goes out of play, captain kicks it back
    // var ballGonePred = function()
    // {
    //     if (team.ball.distance(team.center) > team.radius)
    //     {
    //         return true;
    //     }

    //     return false;
    // };

    // var ballGoneCB = function()
    // {
    //     team.fetchBall(team.captain);
    // };

    // util.create_when(
    //     ballGonePred,
    //     ballGoneCB,
    //     5,
    //     team.ball);


    for (var s=0; s < team.allPlayers.length; ++s)
    {
//         //when ball is near a player, player locks it and tries to kick it.
//         var ballNearPred = function()
//         {
//             if ((team.ball.distance(team.allPlayers[s].getPosition())  < team.playerRadius) &&
//                 (! team.kickLock) &&
//                 (team.ball.distance(team.center) < team.radius))
//             {
//                 return true;
//             }

//             return false;
//         };

//         var ballNearCB = function()
//         {
//             team.kickBall(team.allPlayers[s]);
//         };

//         util.create_when(
//             ballNearPred,
//             ballNearCB,
//             .5,
//             team.allPlayers[s]
//         );

        //player wander
        team.playerWander(team.allPlayers[s]);
    }
}