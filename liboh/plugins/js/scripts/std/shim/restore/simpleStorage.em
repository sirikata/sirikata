
if (typeof(std) == 'undefined')
    std = {};

if (typeof(std.simpleStorage) != 'undefined')
    throw 'Error in simple storage.  Should only import once.';


(function()
{
    system.require('std/shim/restore/persistService.em');
        

    std.simpleStorage = {  };

    var mSelfPres_field = 'mSelfPres';
    std.simpleStorage.setMPres = function(pres)
    {
        std.persist.checkpointPartialPersist(pres,mSelfPres_field);
    };
    
    var mCharacteristics_field = 'mChars';
    std.simpleStorage.setMChars = function(newChars)
    {
        mChars = newChars;
        std.persist.checkpointPartialPersist(newChars,mCharacteristics_field);
    };
    
    std.simpleStorage.restoreMChars = function (cb)
    {
        std.persist.restoreFromAsync(mCharacteristics_field, function(success, obj) {
                                         mChars = obj;
                                         cb(success, obj);
                                     });
    };
    
    std.simpleStorage.fullRestore = function(cb)
    {
        if (system.self != system)
            return;  //means that we've already got a presence.
        
        var restoredCB = function(success, restObjGraph, nServ)
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
            if ((typeof(cb) != 'undefined') && (cb != null))
                cb(mChars);
        };
            
        std.simpleStorage.restoreMChars(
            function() {
                std.persist.restoreFromAsync(mSelfPres_field,restoredCB);
            }
        );
    };

    std.simpleStorage.setRestoreScript = function (cb)
    {
        var scriptString = cb.toString();
        scriptString = 'std.simpleStorage.fullRestore( ' + scriptString + ');';
        var restObj = {
            'restString':scriptString
        };
        std.persist.checkpointPartialPersist(restObj,'restObj');
    };
}
)();
