
if (typeof(std) == 'undefined')
    std = {};

if (typeof(std.simpleStorage) != 'undefined')
    throw 'Error in simple storage.  Should only import once.';


(function()
{
    system.require('std/shim/restore/persistService.em');
    std.simpleStorage = {  };

    var charKeyName   =    'pCharsKeyName';
    var scriptKeyName =   'pScriptKeyName';
    var presKeyName   =     'pPresKeyName';

    var mChars  = { };
    var mScript = { };
    var mPres   = { };
    
    std.simpleStorage.setField = function(fieldName,fieldVal)
    {
        mChars[fieldName] = fieldVal;
        std.persist.checkpointPartialPersist(mChars,charKeyName);
    };

    std.simpleStorage.readField = function(fieldName,cb,def)
    {
        std.persist.restoreFromAsync(charKeyName,
                                     function(success,val)
                                     {
                                         if ((success) && (fieldName in val))
                                         {
                                             mChars[fieldName] = val[fieldName];
                                             cb(val[fieldName]);
                                         }
                                         else
                                             cb(def);
                                     });
    };

    std.simpleStorage.setPresence = function (presToRestore)
    {
        mPres[presToRestore.toString()] =presToRestore;
        std.persist.checkpointPartialPersist(mPres,presKeyName);
    };

    std.simpleStorage.setScript = function(newScriptFunc,executeOnSet)
    {
        var newScript =  ("(" + newScriptFunc + ")();");
        if (typeof(executeOnSet) === 'undefined')
            executeOnSet = false;
        
        mScript.script = newScript;
        system.setRestoreScript('system.require("std/shim/restore/simpleStorage.em");');
        
        var cbFunc = function(){ };
        if (executeOnSet)
            cbFunc = function(){ system.eval(newScript);  };

        std.persist.checkpointPartialPersist(mScript,scriptKeyName,cbFunc);
    };

    
    std.simpleStorage.readScript = function(cb,def)
    {
        std.persist.restoreFromAsync(scriptKeyName,
                                     function(success,val)
                                     {
                                         if ((success) && ('script' in val))
                                             cb(val.script);
                                         else
                                             cb(def);
                                     });
    };


    function onRestoreScript(scriptToEval)
    {
        system.eval(scriptToEval);
    };


    function onRestoreFields()
    {
        finishOnRestoreField();
    }

    function finishOnRestoreField()
    {
        std.simpleStorage.readScript(onRestoreScript,"");        
    }

    std.simpleStorage.readField('someField',onRestoreFields,null);
})();

