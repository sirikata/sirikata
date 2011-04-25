
if (typeof(std) === "undefined") std = {};
if (typeof(std.core) === "undefined") std.core = {};


(function ()
              {
                  var ns = std.core;

                  /**
                   @param an object to perform a deep copy of.
                   
                   Returns a deeply-copied version of argument passed in.  If argument passed in is not an object, just returns that argument directly.  
                   */
                  ns.deepCopy = function(toDeepCopy)
                  {
                      if (typeof(toDeepCopy) !== "object" )
                          return toDeepCopy;
                      

                      var __internalObj = {  };
                      __internalObj.checkAlreadyCopied = function(alreadyCopied,toCheck)
                      {
                          for (var s in alreadyCopied)
                          {
                              if (alreadyCopied[s][0] === toCheck )
                                  return alreadyCopied[s][1];                                  
                              
                          }
                          return null;
                      };

                      __internalObj.deepCopyRecurse = function (toDeepCopy, toCopyTo, alreadyCopied)
                      {
                          for (var s in toDeepCopy )
                          {
                              if (typeof(toDeepCopy[s]) !== "object")
                                  toCopyTo[s] = toDeepCopy[s]; //can just copy by value (what about arrays?)
                              else
                              {
                                  var checkCopied = this.checkAlreadyCopied(alreadyCopied, toDeepCopy[s]);
                                  if (checkCopied != null)
                                  {
                                      //means that the object requested to be copied had already been copied.
                                      toCopyTo[s] = checkCopied;
                                      continue;
                                  }
                              
                                  var newToCopyTo = new Object();
                                  alreadyCopied.push([toDeepCopy[s], newToCopyTo]);
                                  toCopyTo[s] = newToCopyTo;
                                  this.deepCopyRecurse(toDeepCopy[s],newToCopyTo, alreadyCopied);
                              }
                              
                          }
                      };

                      
                      var returner = {   };
                      __internalObj.deepCopyRecurse(toDeepCopy,returner,[toDeepCopy, returner]);
                      
                      return returner;
                  }
              })();

