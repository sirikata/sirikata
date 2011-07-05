

var caps = new util.Capabilities(util.Capabilities.IMPORT);
var whichPresence = system.presences[0];
var newContext = caps.createSandbox(whichPresence,null);
