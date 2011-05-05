cb = function(){ system.print("\n\n\nPrint Test\n\n");};

mPat = new util.Pattern("m","o");

handler = system.registerHandler(cb,mPat,null);

handler.printContents();

//should print a debugging message of some type.
