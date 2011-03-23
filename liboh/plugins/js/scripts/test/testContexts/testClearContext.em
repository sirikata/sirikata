//This script is supposed to test the effects of clearing contexts.
//What should happen: after running this script, it should print
//"Printing repeating function" every 3 seconds.  At the scripting
//prompt, type in newContext.clear().  This should cease the repeating
//printed messages.  Calling testContext() again should throw an error
//(announcing something to the effect that we cannot re-exec on a cleared
//context).



system.import('test/testContexts/testContextImport.em');

system.print("\nYou can test clear now by calling clear on newContext.  Then make sure you get an error when you try to suspend/resume newContext.\n");