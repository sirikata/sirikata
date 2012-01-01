
if (typeof(std) == 'undefined')
    std = {};

if (typeof(std.simpleStorage) != 'undefined')
    throw new Error('Error in simple storage.  Should only import once.');


(function()
{
    system.require('std/shim/restore/persistService.em');
    std.simpleStorage = {  };

    var allKeysKeyname   = 'allKeys';
    var prepender        = 'toPrependWith_';

    var hasNotSetScript  =  true;

    var scriptKeyName    = 'pScriptKeyName';

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

    // Filled in with system-initiated presence
    var mAutomaticPresence;
    var mFinishedPresKeyLookup = false;

    // We need to monitor for new presences that we weren't
    // expecting. If we get one, then the system initiated a
    // connection without asking us. In that case, we shouldn't have
    // anything in storage and should be able to immediately invoke
    // the user's code (giving them the correct system.self). We reset
    // the callback if we find a presence to restore so that we should
    // *never* get in here unless it is due to a system-initiated
    // *connection.
    var clearablePresConn = system.onPresenceConnected(
        function(pres, clearable) {

            //we don't want to fire this callback more than one time.
            clearable.clear();
            // If presence key lookup finished unsuccessfully, we can
            // invoke the user code now, otherwise save system.self so
            // it can be used when that lookup finishes.
            if (mFinishedPresKeyLookup)
                restorePresences();
            else
                mAutomaticPresence = system.self;
        }
    );


    std.simpleStorage.setField = function(fieldName,fieldVal,cb)
    {
        var keyName = prepender + fieldName;
        std.persist.checkpointPartialPersist(fieldVal,keyName,cb);
    };
    std.simpleStorage.write = std.simpleStorage.setField;

    std.simpleStorage.eraseField = function(fieldName, cb)
    {
        system.storageErase(prepender + fieldName, cb);
    };
    std.simpleStorage.erase = std.simpleStorage.eraseField;

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
    std.simpleStorage.read = std.simpleStorage.readField;

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
        mPres[presName] = true;

        // //callback to execute if setPresence works correctly.
        // //essentially all it does is that it begins processing
        // //next presence event.
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
    std.simpleStorage.writePresence = std.simpleStorage.setPresence;

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


        
        system.storageBeginTransaction();
        //remove 
        system.storageErase(presName);
        var toRevertOnFail = mPres[presName];
        delete mPres[presName];
        std.persist.checkpointPartialPersist(mPres,presKeyName, function(){}, true);
        system.storageCommit(function (success)
                            {
                                if (!success)
                                {
                                    mPres[presName] = toRevertOnFail;
                                    system.__debugPrint('\nError in simpleStorage.  Could not delete presence correctly\n');
                                }

                                presOperInProgress = false;
                                processNextPresenceEvent();
                            });
    };



    /**
     Pops the next operation to be processed
     */
    function processNextPresenceEvent()
    {
        if (queuedPresOpers.length == 0)
            return;

        var toProcess = queuedPresOpers.pop();

        if (toProcess[0] == 'add')
            std.simpleStorage.setPresence(toProcess[1]);
        else
            std.simpleStorage.removePresence(toProcess[1]);
    }


    std.simpleStorage.setScript = function(newScriptFunc,executeOnSet)
    {
        hasNotSetScript = false;
        var newScript =  ("( " + newScriptFunc + ")();");
        if (typeof(executeOnSet) === 'undefined')
            executeOnSet = false;

        mScript.script = newScript;
        system.setRestoreScript('system.require("std/shim/restore/simpleStorage.em");');

        var cbFunc = function(){ };
        if (executeOnSet)
            cbFunc = function(){ eval(newScript);  };
        
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
        eval(scriptToEval);
    };

    
    //restores the script
    function finishOnRestorePresences()
    {
        if (hasNotSetScript)
            std.simpleStorage.readScript(onRestoreScript,"");
    }


    /**
     Called when we regain our list of presences.
     Initiates restoring each presence separately.
     */
    function onPresKeyRestored(success,allPres)
    {
        // Only continue if we restored presences. Otherwise, we're
        // using an system-initiated presence and
        // system.onPresenceConnected path will make sure the user
        // code gets invoked.
        if (success) {
            // Disable onPresenceConnected: should not be getting
            // system-initiated connection if we have our own in storage.
            clearablePresConn.clear();
            mPres = allPres;
            restorePresences();
        } else {
            // We may or may not have received the onPresenceConnected
            // yet. If we did, trigger the callback, otherwise, mark a
            // flag so the onPresenceConnected callback can trigger
            // it.
            if (mAutomaticPresence) {
                system.changeSelf(mAutomaticPresence);
                restorePresences();
            }
            else
                mFinishedPresKeyLookup = true;
        }
    }

    //keeps track of all the presences that we have already restored.
    var allRestored = {};

    //called each time
    function restorePresences()
    {
        // Callback & data to handle synchronizing all the parallel restoration + connection requests
        var num_outstanding = 0;
        var finishRestorePresence = function(presName,success,pres) {
            if (!success) {
                throw new Error('Failed to restore or connect presence ' + presName + ". Bailing on restoration -- this object's script will *not* be executed.");
            }

            num_outstanding--;
            if (num_outstanding == 0) {
                finishOnRestorePresences();
            }
        };

        // Fire off requests for any presences not yet connected
        for (var s in mPres) {
            //if we've already restored this presence ignore it.
            if (s in allRestored)
                continue;
            num_outstanding++;
            allRestored[s] = true;
            std.persist.restoreFromAsync(s,std.core.bind(finishRestorePresence,undefined,s));
        }
        
        // Special case: no presences to restore.
        if (num_outstanding == 0) finishOnRestorePresences();
    }

    std.persist.restoreFromAsync(presKeyName,onPresKeyRestored);

})();
