

function onReset()
{
    system.require('std/shim/restore/persistService.em');

    var mSelfPres_field = 'mSelfPres';
    setMPres = function(pres)
    {
        std.persist.checkpointPartialPersist(pres,mSelfPres_field);
    };

    var mCharacteristics_field = 'mChars';
    setMChars = function(newChars)
    {
        mChars = newChars;
        std.persist.checkpointPartialPersist(newChars,mCharacteristics_field);
    };

    restoreMChars = function (cb)
    {
        std.persist.restoreFromAsync(mCharacteristics_field, function(success, obj) {
                                         mChars = obj;
                                         cb(success, obj);
                                     });
    };
    
    ( function()
      {
          if (system.self != system)
              return;  //means that we've already got a presence.
          
          var  restoredCB = function(success, restObjGraph, nServ)
          {
              if (! success)
              {
                  throw 'Error: unsuccesful restore\n';
                  system.print('\n\nError: unsuccesful restore\n');
                  system.import('std/defaultAvatar.em');
                  return;
              }
              system.require('std/script/scriptable.em');
              system.require('std/graphics/default.em');
              scriptable = new std.script.Scriptable();
              system.self.setQueryAngle(.00001);      
              simulator = new std.graphics.DefaultGraphics(restObjGraph,'ogregraphics');
          };
          
          restoreMChars(function() {
                            std.persist.restoreFromAsync(mSelfPres_field,restoredCB);
                        });
      })();
}



function setRestoreScript(restoreScript)
{
    system.require('std/shim/restore/persistService.em');
    var scriptString = restoreScript.toString();
    scriptString = '( ' + scriptString + ' )();';
    var restObj = {
        'restString':scriptString
    };
    std.persist.checkpointPartialPersist(restObj,'restObj');
    eval(scriptString);
}


setRestoreScript(onReset);
