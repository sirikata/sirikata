cb = function(){ system.print("\n\n\nPrint Test\n\n")};

mPat = system.Pattern("m");

handler = system.registerHandler(mPat,null,cb);

//handler.printContents();

//should print a debugging message of some type.


