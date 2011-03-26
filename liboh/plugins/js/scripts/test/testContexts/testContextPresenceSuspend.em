//This script is supposed to test that sane things happen to a context's presences
//when that context is suspended, resumed, and cleared.  Among the things that
//So, we create a new context, then create a new presence in that context.  Then
//start moving that presence in a direction.  Then at the command line, we should call
//suspend, resume, suspend, and clear on the context.  This should make it so that
//the new presence moves, stops, moves, stops, and disappears.


system.import('test/testContexts/testProxEventsContext.em');

