cb = function(){ system.print("\n\n\nPrint Test\n\n")};

mPat = new system.Pattern("m","o","oops");
//mPat = system.Pattern("m");

system.print("\n\ncreated pattern\n");

mHand = system.registerHandler(mPat,null,cb,null);

system.print("\n\nregistered pattern\n");

mHand.printContents();
system.print("\n\nprinted contents.\n\n");

tmp = Object();
tmp.m = "o";

system.__broadcast(tmp);

system.print("\n\nbroadcast pattern\n");

