cb = function(){ system.print("\n\n\nPrint Test\n\n")};

mPat = new system.Pattern("m");

handler = system.registerHandler(mPat,null,cb,null);
system.print("\n\nBefore suspended\n");
handler.printContents();

system.print("\nSuspending\n");
handler.suspend();
handler.printContents();

tmp = Object();
tmp.m = "o";

system.__broadcast(tmp); //should not see any message from this
