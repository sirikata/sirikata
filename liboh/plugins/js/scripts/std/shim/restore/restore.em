if (typeof(std.persist) === 'undefined')
    throw 'Error.  Do not import this file directly.  Only import it from persistService.em';



/**
 Reads in a filename as a string.  Calls deserialize on that string to
 get back an object.  Runs through that object graph, and uses
 nameService to restore it.

 @param {String} name of file to read a serialized object graph in
 from.
 */
function restoreFrom(filename)
{
    var serializedGraph = system.__debugFileRead(filename);
    var deserializedGraph = system.deserialize(serializedGraph);

    system.prettyprint(deserializedGraph);
}
