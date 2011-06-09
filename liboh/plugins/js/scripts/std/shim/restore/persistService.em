
if (typeof(std) === "undefined") /** @namespace */ std = {};


(function()
{
     /* Type constants */
     //use this as an index to retrieve the type of local objects
     var GET_TYPE_STRING                   =            '__getType';
    
    std.persist = {

     //set the type field of an object to this if have a loop.
     SYSTEM_TYPE_STRING            :               'system',
     PRESENCE_OBJECT_TYPE_STRING   :             'presence',
     VISIBLE_TYPE_STRING           :              'visible',
     TIMER_TYPE_STRING             :                'timer',
     PRESENCE_ENTRY_TYPE_STRING    :        'presenceEntry',
     FUNCTION_OBJECT_TYPE_STRING   :   'funcObjectAsString',
     BASIC_OBJECT_TYPE_STRING      :          'basicObject',
     VEC_TYPE_STRING               :                 'vec3',
     QUAT_TYPE_STRING              :                 'quat',

        
        /**
         @param obj to check if it is a vec object
         @return true if is vec object false otherwise
         */
        checkVec3 : function (obj)
        {
            if (GET_TYPE_STRING in obj)
                return (obj[GET_TYPE_STRING]() == std.persist.VEC_TYPE_STRING);

            return false;
        },

        /**
         @param obj to check if it is a quat object
         @return true if is quat object false otherwise
         */
        checkQuat : function (obj)
        {
            if (GET_TYPE_STRING in obj)
                return (obj[GET_TYPE_STRING]() == std.persist.QUAT_TYPE_STRING);

            return false;
        },
        
        
    /**
     @param obj to check if it is a system object
     @return true if system object.  false otherwise
     */
    checkSystem : function (obj)
    {
        if (GET_TYPE_STRING in obj)
            return (obj[GET_TYPE_STRING]() == std.persist.SYSTEM_TYPE_STRING);
        
        return false;
    },

    /**
     @param obj to check if it is a function object
     @return true if function object.  false otherwise
     */
    checkFunction : function (obj)
    {
        if (typeof(obj) == 'function')
            return true;
        return false;
    },

        
    /**
     @param obj to check if it is a presence object
     @return true if presence object.  false otherwise
     */
    checkPresence : function (obj)
    {
        if (GET_TYPE_STRING in obj)
            return (obj[GET_TYPE_STRING]() == std.persist.PRESENCE_OBJECT_TYPE_STRING);
        
        return false;
    },

    /**
     @param obj to check if it is a visible object
     @return true if visible object.  false otherwise
     */
    checkVisible : function (obj)
    {
        if (GET_TYPE_STRING in obj)
            return (obj[GET_TYPE_STRING]() == std.persist.VISIBLE_TYPE_STRING);
        
        return false;
    },

    /**
     @param obj to check if it is a timer object
     @return true if timer object.  false otherwise
     */
    checkTimer : function (obj)
    {
        if (GET_TYPE_STRING in obj)
            return (obj[GET_TYPE_STRING]() == std.persist.TIMER_TYPE_STRING);

        return false;
    },

    
    /**
     @param obj to check if it is a PresenceEntry object
     @return true if PresenceEntry object.  false otherwise
     */
    checkPresenceEntry : function (obj)
    {
        if (GET_TYPE_STRING in obj)
            return (obj[GET_TYPE_STRING]() == std.persist.PRESENCE_ENTRY_TYPE_STRING);
        
        return false;
    },


    
    getValueFromPropValPair : function (obj)
    {
        if (typeof(obj) != 'object')
            throw 'Error in getValueFromPropValPair.  Requires an object for an argument';
        
        if (!('length' in obj))
            throw 'Error in getValueFromPropValPair.  Any fields received to decode should be arrays.';     

        return obj[1];
    },

    getPropFromPropValPair : function (obj)
    {
        if (typeof(obj) != 'object')
            throw 'Error in getPropFromPropValPair.  Requires an object for an argument';
        
        if (!('length' in obj))
            throw 'Error in getPropFromPropValPair.  Any fields received to decode should be arrays.';

        return obj[0];
    },

    wrapPropValPair : function (prop,val)
    {
       return [prop,val];
    },

    /**
     @param toCheck.  Variable to check if it's in the form of a propertyValuePair.
     @returns true if param is in form of a propertyValuePair.  Returns false otherwise.
     */
    isPropValPair : function (toCheck)
    {
        return ((typeof(toCheck) === 'object') && ('length' in toCheck) && (toCheck.length == 2));
    },
    
    /**
     @param {array} pair.  Takes an array of size two in
     (corresponding to a wrappedPropValPair).  If input is not in this
     form, throws error.
     @return Object if the pair points to an object.  False otherwise.
     */
    propValPointsToObj : function(pair)
    {
        if (! std.persist.isPropValPair(pair))
            throw 'Error.  Requires an array of length two to be passed in.';

        if (typeof(std.persist.getPropFromPropValPair(pair)) === 'object')
            return std.persist.getPropFromPropValPair(pair);

        return false;
    }
    };
    })();


system.require('std/shim/restore/nameService.em');
system.require('std/shim/restore/persist.em');
system.require('std/shim/restore/restore.em');
system.require('std/shim/restore/record.em');
system.require('std/shim/restore/backend.em');
