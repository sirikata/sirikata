//This script shows how to create a new entity.
//The new entity's presence will have the mesh pointed to by
//meshToChangeTo (a sphere) and will be placed 2 units away from
//the presence that is creating it.
//The first thing that the new entity does is it imports the script
//pointed to by the scriptToImport variable (just prints a message).
//Note: you may have to check the console runing the program to
//receive this message instead of the in-world scripting prompt.


//new entity's presence has the mesh of a sphere
var meshToChangeTo = "meerkat:///test/sphere.dae/original/0/sphere.dae";

//new entity's presence will be located two units away from creating
//presence's position
var newPos = system.presences[0].getPosition();
newPos.x = newPos.x + 2;

//First thing that the new entity will do after its presence connects
//to space is import the following file.
var scriptToImport = "examples/testPrint.em";



//system.create_entity(newPos,
system.create_entity(newPos,
                     "js",    //this arg will almost always be 'js'
                     scriptToImport,
                     meshToChangeTo,
                     1.0,     //how do you want to scale the mesh of
                              //the entity's new presence
                     3        //what is the solid angle query that
                              //the entity's initial presence
                              //queries with.
                    );

