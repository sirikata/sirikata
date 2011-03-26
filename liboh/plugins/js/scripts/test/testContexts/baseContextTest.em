
var whichPresence = system.presences[0];
var newContext = system.create_context(whichPresence,null,
                                       true,  //can I send to everyone
                                       true,  //can I receive messages from everyone
                                       true,  //can I make my own prox queries
                                       true,  //can I import
                                       true,  //can I create presences
                                       true,  //can I create entities
                                       true   //can I call eval directly 
                                      );
