cb = function()
{
    system.print("\n\n\nPrint Test\n\n");
};

mPat = new util.Pattern("m");

handler = system.registerHandler(cb,mPat,null);


system.print("\n\nBefore suspended\n");
handler.printContents();

system.print("\nSuspending\n");
handler.suspend();
handler.printContents();

system.print("\n\nCreate another scripted entity and call testSendMessageVisible.em on it.\n\n");






//deprecated broadcast
//tmp = Object();
//tmp.m = "o";
//system.__broadcast(tmp); //should not see any message from this
