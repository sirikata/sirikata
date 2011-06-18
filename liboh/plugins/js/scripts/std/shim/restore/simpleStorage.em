
if (typeof(std) == 'undefined')
    std = {};

if (typeof(std.simpleStorage) != 'undefined')
    throw 'Error in simple storage.  Should only import once.';


(function()
{
    system.require('std/shim/restore/persistService.em');
    std.simpleStorage = {  };

    var allKeysKeyname = 'allKeys';
    var prepender      = 'toPrependWith_';


    var scriptKeyName =   'pScriptKeyName';

    //contains the script to execute on restore in its script field.
    var mScript = { };

    
    //query backend with this index to retrieve mPres object, which lists
    //all presences that we have.
    var presKeyName   =     'pPresKeyName';
    //prevents more than one presence from being stored at a time because
    //we didn't expose batching erases and sets in the same set unfortunately
    var presOperInProgress = false;
    //last elements of the array contain next elements to be processed
    var queuedPresOpers= [];
    //Keys contain all presence sporefs.  Can use these to restore our presences.
    var mPres   = { };        
    

    
    std.simpleStorage.setField = function(fieldName,fieldVal)
    {
        var keyName = prepender + fieldName;
        std.persist.checkpointPartialPersist(fieldVal,keyName);            
    };

    std.simpleStorage.eraseField = function(fieldName)
    {
        system.storageErase(prepender + fieldName);
    };
    
    std.simpleStorage.readField = function(fieldName,cb,def)
    {
        std.persist.restoreFromAsync(prepender + fieldName,
                                     function(success,val)
                                     {
                                         if (success)
                                             cb(val);
                                         else
                                             cb(def);
                                     });
    };

    
    std.simpleStorage.setPresence = function (presToRestore)
    {
        //cannot process current presence because already performing a
        //presence operation.
        if (presOperInProgress)
        {
            queuedPresOpers.unshift(['add',presToRestore]);
            return;
        }

        //locks other presence operations from occurring.
        presOperInProgress = true;

        //ensures this presence is added to our list of presences
        var presName = presToRestore.toString();
        //var presName = 'a';//presToRestore.toString();        
        mPres[presName] = true;
        
        //callback to execute if setPresence works correctly.
        //essentially all it does is that it begins processing
        //next presence event.
        var finishedCB = function(success)
        {
            if (!success)
                delete mPres[presName];

            presOperInProgress = false;
            processNextPresenceEvent();
        };

        
        std.persist.persistMany([[mPres,presKeyName],
                                [presToRestore,presName]], finishedCB);
        
    };


    std.simpleStorage.erasePresence = function (presToErase)
    {
        //cannot process current presence because already performing a
        //presence operation.
        if (presOperInProgress)
        {
            queuedPresOpers.unshift(['remove',presToErase]);
            return;
        }

        //locks other presence operations from occurring.
        presOperInProgress = true;

        var presName = presToRestore.toString();        

        //FIXME: currently cannot batch erase and write operations simultaneously.
        //As a result, mPres and actual presences in backend could get out of sync.
        //When this bug is resolved, the following finished cb will look a lot more
        //like the one in the above func.
        var finishedErasedPres = function(success)
        {
            var finishedUpdatedMPres = function(success)
            {
                if (!success)
                {
                    system.print('\nError in simpleStorage.  Could not delete presence correctly\n');
                }
                presOperInProgress = false;
                processNextPresenceEvent();
            };
            
            
            if (success)
            {
                delete mPres[presName];
                std.persist.checkpointPartialPersist(mPres,presKeyName);
                return;
            }
            presOperInProgress = false;
            processNextPresenceEvent();
        };

        system.storageErase(presName,finishedErasePres);
    };

    

    /**
     Pops the next operation to be processed
     */
    function processNextPresenceEvent()
    {
        if (queuedPresOpers.length == 0)
            return;

        var nextToProcess = queuedPresOpers.length -1;

        var toProcess = queuedPresOpers[nextToProcess];
        delete queuedPresOpers.nextToProcess;

        if (toProcess[0] == 'add')
            std.simpleStorage.setPresence(toProcess[1]);
        else
            std.simpleStorage.removePresence(toProcess[1]);
    }

    
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


    //evaluates the script once it has been restored.
    function onRestoreScript(scriptToEval)
    {
        system.eval(scriptToEval);
    };

    //restores the script
    function finishOnRestorePresences()
    {
        std.simpleStorage.readScript(onRestoreScript,"");        
    }



    /**
     Called when we regain our list of presences.
     Initiates restoring each presence separately.
     */
    function onPresKeyRestored(success,allPres)
    {
        if (success)
            mPres = allPres;
        else
            system.print('\nFailure restoring presences\n');
        
        system.print('\n\nThese are all presences\n\n');
        system.prettyprint(mPres);
        system.print('\n\n');
        
        restorePresences();
    }

    //keeps track of all the presences that we have already restored.
    var allRestored = {};

    //called each time 
    function restorePresences(success,pres)
    {
        if (typeof(success) !== 'undefined' )
        {
            system.print('\nThis was success: \n');
            system.print(success);
            system.print('\n\n');
        }
        system.print('\nGot into restorePresences\n\n');
        for (var s in mPres)
        {
            //if we've already restored this presence ignore it.
            if (s in allRestored)
                continue;

            //restore this presence, but call return so that we
            //don't yet restore script (in finishOnRestorePresences).
            allRestored[s] = true;
            std.persist.restoreFromAsync(s,restorePresences);
            return;
        }

        //if had no presences to restore, or they all had already been restored,
        //will call finishOnRestorePresences(), which just restores the script.
        finishOnRestorePresences();
    }

    std.persist.restoreFromAsync(presKeyName,onPresKeyRestored);

})();

