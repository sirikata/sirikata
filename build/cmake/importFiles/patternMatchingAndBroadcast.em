cb = function(){ system.print("\n\n\nPrint Test\n\n")};

mPat = system.Pattern("m");

system.registerHandler(mPat,null,cb);


tmp = Object();

tmp.m = "I have an m field";



system.__broadcast(tmp);


