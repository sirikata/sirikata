

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

function setVelocityToCenter(ball,center,speed)
{
    var ballPos = ball.getPosition();
    var unNormalizedVeloc = new util.Vec3(center.x - ballPos.x, center.y - ballPos.y, center.z - ballPos.z);
    var normalizedVeloc = unNormalizedVeloc.normal();
    system.print("\n\nunnormal: " + unNormalizedVeloc + "\n\n");
    system.print("\nnormal: " + normalizedVeloc + "\n\n");
    
    normalizedVeloc.x = normalizedVeloc.x*speed;
    normalizedVeloc.y = normalizedVeloc.y*speed;
    normalizedVeloc.z = normalizedVeloc.z*speed;
    ball.setVelocity(normalizedVeloc);
}



function createTeamObject(ball)
{
    var team = util.create_watched();
    team.count = 0;
    team.maxPlayers = 11;
    team.kickLock = false;
    team.playerRadius = 3;
    
    team.playerMesh = "meerkat:///danielrh/Poisonbird.dae";
    team.ball = ball;
    team.center = ball.getPosition();
    team.radius = 15;
    team.playerSpeed = .5;
    team.ballSpeed = 1;
    team.allPlayers = new Array();
    team.captain = null;
    team.addPlayer = function(newPlayer)
    {
        team.allPlayers.push(newPlayer);
        newPlayer.whichPlayer = team.count;
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
            setVelocityToCenter(playerToWander,team.center,team.playerSpeed);
        }
        else
        {
            angle = baseAngle + (maxAngle - baseAngle)*util.rand();
            setVelocity(playerToWander,angle,team.playerSpeed);
        }            

        system.timeout(team.playerWanderPeriod,null,
                       function ()
                       {
                         team.playerWander(playerToWander);
                       });
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
        setVelocityToCenter(team.ball,team.center,team.ballSpeed);
        team.kickLock = false;
    };

    return team;
}

mTeam = createTeamObject(system.presences[0]);
function waitForJoin(presJoined)
{
    var predicate = function()
    {
        return presJoined.isConnected;
    };

    var callbacker = function()
    {
        mTeam.addPlayer(presJoined);
    };

    util.create_when(
        predicate,
        callbacker,
        5,
        presJoined);
}

teamArray = new Array();
for (var s=0; s < mTeam.maxPlayers; ++s)
{
    teamArray.push(system.create_presence(mTeam.playerMesh,presAdded));
    waitForJoin(teamArray[s]);
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




function ballNearKick(player,team)
{
    //when ball is near a player, player locks it and tries to kick it.
    var ballNearPred = function()
    {
        if ((team.ball.distance(player.getPosition())  < team.playerRadius) &&
            (! team.kickLock) &&
            (team.ball.distance(team.center) < team.radius))
        {

            return true;
        }

        return false;
    };

    var ballNearCB = function()
    {
        team.kickBall(player);
    };

    util.create_when(
        ballNearPred,
        ballNearCB,
        .5,
        player);
}

function playSoccer(team)
{
    //get players to wander around
    for (var s=0; s < team.allPlayers.length; ++s)
    {
        var whichPlayer = team.allPlayers[s].whichPlayer;
        system.print("\n\nBeginning wander for player" + whichPlayer+ "\n\n");
        team.playerWander(team.allPlayers[s]);            
    }

    //when ball is near player, player kicks it.
    for (var t=0; t < team.allPlayers.length; ++t)
    {
        ballNearKick(team.allPlayers[t],team);
    }

    //when ball goes out of play, captain kicks it back
    var ballGonePred = function()
    {
        if (team.ball.distance(team.center) > team.radius)
        {
            return true;
        }

        return false;
    };

    var ballGoneCB = function()
    {
        team.fetchBall(team.captain);
    };

    util.create_when(
        ballGonePred,
        ballGoneCB,
        5,
        team.ball);
    
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