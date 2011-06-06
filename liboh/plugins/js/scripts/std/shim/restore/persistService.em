


if (typeof(std) === "undefined") /** @namespace */ std = {};


std.persist = {

    
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


system.require('std/shim/restore/nameService.em');
system.require('std/shim/restore/persist.em');
system.require('std/shim/restore/restore.em');
system.require('std/shim/restore/record.em');
system.require('std/shim/restore/backend.em');
