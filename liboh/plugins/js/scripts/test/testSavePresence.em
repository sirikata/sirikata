


system.require('std/shim/restore/persistService.em');
var fName = 'presToPersist';
var nameServe = std.persist.checkpointPartialPersist(system.self,fName);
