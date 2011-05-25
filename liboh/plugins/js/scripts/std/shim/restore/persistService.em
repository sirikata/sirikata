


if (typeof(std) === "undefined") /** @namespace */ std = {};


std.persist = {

    NO_RESTORE_STRING : '__noRestore',

    //each object being persisted is labeled with an id.  This id is
    //provided by the name service. The id provides for loops.
    //this is how to index into the object to get its id
    ID_FIELD_STRING   : '__hidden_id',

    
    //each object in the serialized graph has a type.  This is the
    //field for that type in the shadow graph.
    TYPE_FIELD_STRING : '__hidden_type',


    /*Type Constants*/

    //use this as an index to retrieve the type of an object
    GET_TYPE_STRING                   :            '__getType',
    

    //set the type field of an object to this if have a loop.
    POINTER_OBJECT_STRING             :     '__pointer_object',  
    SYSTEM_TYPE_STRING                :               'system',
    PRESENCE_OBJECT_TYPE_STRING       :             'presence',
    VISIBLE_TYPE_STRING               :              'visible',
    TIMER_TYPE_STRING                 :                'timer',
    PRESENCE_ENTRY_TYPE_STRING        :        'presenceEntry',
    FUNCTION_OBJECT_TYPE_STRING       :   'funcObjectAsString',


    //for pointer objects, use this as the index of a pointer object
    //to get back the unique name of the object that the pointer object
    //is supposed to point to.  The name is provided by nameService, and
    //can pass it to nameService to get back the object reference.
    POINTER_FIELD_STRING  : '__pointer_field',


    //create a NonRestorable type.  When running through object graph,
    //will not persist objects that are marked as nonRestorable.
    NonRestorable : function ()    
    {
        this[NO_RESTORE_STRING] = true;
    },

    checkNonRestorable : function (obj)
    {
        if (typeof(obj)   == 'object')
            return (std.persist.NO_RESTORE_STRING in obj);

        return false;
    },


    /**
     @param obj to check if it is a system object
     @return true if system object.  false otherwise
     */
    checkSystem : function (obj)
    {
        if (std.persist.GET_TYPE_STRING in obj)
            return (obj[std.persist.GET_TYPE_STRING]() == std.persist.SYSTEM_TYPE_STRING);
        
        return false;
    },

    /**
     @param obj to check if it is a presence object
     @return true if presence object.  false otherwise
     */
    checkPresence : function (obj)
    {
        if (std.persist.GET_TYPE_STRING in obj)
            return (obj[std.persist.GET_TYPE_STRING]() == std.persist.PRESENCE_TYPE_STRING);
        
        return false;
    },

    /**
     @param obj to check if it is a visible object
     @return true if visible object.  false otherwise
     */
    checkVisible : function (obj)
    {
        if (std.persist.GET_TYPE_STRING in obj)
            return (obj[std.persist.GET_TYPE_STRING]() == std.persist.VISIBLE_TYPE_STRING);
        
        return false;
    },

    /**
     @param obj to check if it is a timer object
     @return true if timer object.  false otherwise
     */
    checkTimer : function (obj)
    {
        if (std.persist.GET_TYPE_STRING in obj)
            return (obj[std.persist.GET_TYPE_STRING]() == std.persist.TIMER_TYPE_STRING);

        return false;
    },

    /**
     @param obj to check if it is a PresenceEntry object
     @return true if PresenceEntry object.  false otherwise
     */
    checkPresenceEntry : function (obj)
    {
        if (std.persist.GET_TYPE_STRING in obj)
            return (obj[std.persist.GET_TYPE_STRING]() == std.persist.PRESENCE_ENTRY_TYPE_STRING);
        
        return false;
    },

    
    /**
     Use when serializing data.
     @param func to check if it is a function object
     @return true if function object.  false otherwise
     */
    checkFunctionObject : function (func)
    {
        return (typeof(func) == 'function');
    },

    /**
     Use when deserializing data.
     @param func to check if it is a function object
     @return true if function object.  false otherwise
     */
    checkFunctionSerial : function (func)
    {
        if (std.persist.GET_TYPE_STRING in obj)
            return (obj[std.persist.GET_TYPE_STRING]() == std.persist.FUNCTION_TYPE_STRING);

        return false;
    }
    
};


system.require('std/shim/restore/nameService.em');
system.require('std/shim/restore/persist.em');
system.require('std/shim/restore/restore.em');
