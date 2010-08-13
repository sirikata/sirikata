//assume have already gotten logged into space

//HIGH-LEVEL explanation:
//This file is to test the getPosition and setPosition functions fromx presences.
//Whenever someone sends the scripted object a message with an object that has a field
//called "tremble", we call the tremble function.


centerPosition     = new system.presences[0].getPosition();

// awayFromCenterBool = false;
// displacementVector = new system.Vec3(1.0,1.0,1.0);

// function tremble_callback_function()
// {
//     var currentPosition = system.presences[0].getPosition();
//     if (awayFromCenterBool)
//         system.presences[0].setPosition(currentPosition + displacementVector);
//     else
//         system.presences[0].setPosition(currentPosition - displacementVector);

// }

// var matchTremblePattern = new system.Pattern("tremble");

// var trembleHandler      = new system.registerHandler(matchPattern,null,tremble_callback_function,null);

// var objer = new Object();
// objer.tremble = true;


// function callTrembler()
// {
//     system.__broadcast(objer);
// }


