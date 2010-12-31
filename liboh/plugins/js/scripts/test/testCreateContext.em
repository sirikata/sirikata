///This function tests the createContext code.  From system, create a context, print in the context, and execute from the new context


system.print("\n\nRunning test/testCreatecontext.em.  Tests creating a context, printing from the context, executing a function with no args, and executing a function with args \n\n");


system.print("\nCreate context\n");

var newContext = system.create_context();

newContext.print("\n\nPRINTING FROM NEW_CONTEXT\n\n");