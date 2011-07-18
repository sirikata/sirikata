
//note: this file is imported as js.  All js operators are valid here,
//but things like +,-, etc. aren't over-written.


//note: this is just a documentation hack to get jsdoc to pick up util documentation.
if (typeof(util) == 'undefined')
{
    /** @namespace */
    util = {};
}


(function()
{
    var internalUtilRand = std.core.bind(util.rand,util);
    var internalUtilSqrt = std.core.bind(util.sqrt,util);
    var internalUtilIdentifier = std.core.bind(util.identifier,util);

    var internalUtilAcos = std.core.bind(util.acos,util);
    var internalUtilCos = std.core.bind(util.cos,util);
    var internalUtilSin = std.core.bind(util.sin,util);
    var internalUtilAsin  = std.core.bind(util.asin,util);
    var internalUtilPow = std.core.bind(util.pow,util);
    var internalUtilExp = std.core.bind(util.exp,util);
    var internalUtilAbs = std.core.bind(util.abs,util);

    var internalUtilPattern = util.Pattern;
    var internalUtilVec3 = util.Vec3;
    var internalUtilQuaternion = util.Quaternion;

    var internalBase64Encode = util._base64Encode;
    var internalBase64EncodeURL = util._base64EncodeURL;
    var internalBase64Decode = util._base64Decode;
    var internalBase64DecodeURL = util._base64DecodeURL;
    delete util._base64Encode;
    delete util._base64EncodeURL;
    delete util._base64Decode;
    delete util._base64DecodeURL;

    /**
     Overloads the '+' operator.

     Checks if lhs has a plus function defined on it.  If it does, then
     calls lhs.plus(rhs).  If it doesn't, call javascript version of +
     */
    util.plus = function (lhs, rhs)
    {
        if ((lhs != null) && (typeof(lhs) != 'undefined'))
        {
            if (typeof(lhs.add) == 'function')
                return lhs.add(rhs);
        }

        return lhs + rhs;
    };


    /**
     Overloads the '-' operator.
     
     Checks if lhs has a sub function defined on it.  If it does, then
     calls lhs.sub(rhs).  If it doesn't, javascript version of -.
     */
    util.sub = function (lhs,rhs)
    {
        if ((lhs != null)&&(typeof(lhs) != 'undefined'))
        {
            if (typeof(lhs.sub) == 'function')
                return lhs.sub(rhs);
        }
        
        return lhs - rhs;
    };

    /**
     Overloads the '/' operator.

     Checks if lhs has a div function defined on it.  If it does, then
     returns lhs.div(rhs).  Otherwise, returns normal js division.
     */
    util.div = function (lhs,rhs)
    {
        if ((lhs != null)&&(typeof(lhs) != 'undefined'))
        {
            if (typeof(lhs.div) == 'function')
                return lhs.div(rhs);
        }
        
        return lhs/rhs;
    };

    /**
     Overloads the '*' operator.

     Checks if lhs has a mul function defined on it.  If it does, then
     returns lhs.mul(rhs).  Otherwise, returns normal js *.
     */
    util.mul = function (lhs,rhs)
    {
        if ((lhs != null) && (typeof(lhs) != 'undefined'))
        {
            if (typeof(lhs.mul) == 'function')
                return lhs.mul(rhs);
        }
        
        return lhs* rhs;
    };

    /**
     Overloads the '%' operator.

     Checks if lhs has a mod function defined on it.  If it does, then
     returns lhs.mod(rhs).  Otherwise, returns normal js modulo.
     */
    util.mod = function (lhs,rhs)
    {
        if ((lhs != null) && (typeof(lhs) != 'undefined'))
        {
            if (typeof(lhs.mod) == 'function')
                return lhs.mod(rhs);
        }
        
        return lhs % rhs;
    };


    util.equal = function (lhs,rhs)
    {
        if ((lhs !=null) && (typeof(lhs) != 'undefined'))
        {
            if (typeof(lhs.equal)  =='function')
                return lhs.equal(rhs);
        }
        
        return (lhs == rhs);
    };

    util.notEqual = function (lhs,rhs)
    {
        if ((lhs != null)&& (typeof(lhs) != 'undefined'))
        {
            if (typeof(lhs.notEqual) == 'function')
                return lhs.notEqual(rhs);
        }
        
        return (lhs != rhs);
    };

    util.identical = function (lhs,rhs)
    {
        if ((lhs !== null) && (typeof(lhs) != 'undefined'))
        {
            if (typeof(lhs.identical) == 'function')
                return lhs.identical(rhs);
        }
        
        return (lhs === rhs);
    };

    util.notIdentical = function (lhs,rhs)
    {
        if ((lhs != null) && (typeof(lhs) != 'undefined'))
        {
            if (typeof(lhs.notIdentical) == 'function')
                return lhs.notIdentical(rhs);
        }
        
        return (lhs !== rhs);
    };
    
    
      /**
       @return a random float from 0 to 1
       */
      util.rand = function()
      {
          return internalUtilRand.apply(undefined,arguments);
      };


      /**
       @param takes in a single argument
       @return returns a float
       */
      util.sqrt = function()
      {
          return internalUtilSqrt.apply(undefined,arguments);
      };

      /**
       @param {optional} {string} space id
       @param {optional} {string} presence id

       @return {string} without any arguments, returns a string with
       default null values for both space identifier and presence
       identifier.  With single arg, returns identifier with
       argument's space id, but with a null presence identifier.  With
       two args, returns full id as string with both space id and
       presence id.
       
       */
      util.identifier = function()
      {
          return internalUtilIdentifier.apply(undefined,arguments);
      };
      
      /**
       @param float to take arccosine of
       @return angle in radians
       */
      util.acos = function()
      {
          return internalUtilAcos.apply(undefined,arguments);
      };

      /**
       @param angle in radians to take cosine of
       @return cosine of that angle
       */
      util.cos = function()
      {
          return internalUtilCos.apply(undefined,arguments);
      };


      /**
       @param angle in radians to take sine of
       @return sine of that angle
       */
      util.sin = function()
      {
          return internalUtilSin.apply(undefined,arguments);
      };


      /**
       @param float to take arcsine of
       @return angle in radians
       */
      util.asin = function()
      {
          return internalUtilAsin.apply(undefined,arguments);
      };


      /**
       @param base
       @param exponent
       @return returns base to the exponent
       */
      util.pow = function()
      {
          return internalUtilPow.apply(undefined,arguments);
      };


      /**
       @param exponent
       @return returns e to the exponent
       */
      util.exp = function()
      {
          return internalUtilExp.apply(undefined,arguments);
      };

      /**
       @param number to take abs of
       @return returns absolute value of argument.
       */
      util.abs = function()
      {
          return internalUtilAbs.apply(undefined,arguments);
      };


      /**
       @param name to match
       @param value to match (can be null)
       @param proto to match (can be null)

       @return pattern object
       */
      util.Pattern =function()
      {
          return internalUtilPattern.apply(this,arguments);
      };


    
      /**
       @namespace 
       
       @param x
       @param y
       @param z
       @param w

       Quaternion should be assumed to only have .x, .y, .z, and .w. 
       
       @return a quaternion object
       */
      util.Quaternion = function()
      {
          return internalUtilQuaternion.apply(this,arguments);
      };

      /**
       @namespace
       
       @param x
       @param y
       @param z

       @return a vec3 object
        Vec3 should be assumed to only have .x, .y, and .z. 
       */
      util.Vec3 = function()
      {
          return internalUtilVec3.apply(this,arguments);
      };
    
    /** @namespace Utilities for converting between binary
     *  arrays/strings and base64 encoded strings.
     */
    util.Base64 = {};


    /** Encode binary data as a Base64 string. This does not handle
     *  URL escaping, i.e. special characters such as '=' may exist in
     *  the returned string.
     *  @param {string|array} value a string or array of data to encode
     *  @returns {string} base 64 encoded data
     *  @throws {Error} if the parameter is invalid (empty or not a valid type).
     */
    util.Base64.encode = function(value) {
        if (typeof(value) === "string")
            return internalBase64Encode(value);
        throw new Error('Invalid type passed to Base64.encode.');
    };
    
    /** Encode binary data as a Base64 string. This version encodes
     *  for URLs, so special characters like '=' are escaped.
     *  @param {string} value a string or array of data to encode. It will use utf-8 encoding.
     *  @returns {string} base 64 encoded data
     *  @throws {Error} if the parameter is invalid (empty or not a valid type).
     */
    util.Base64.encodeURL = function(value) {
        if (typeof(value) === "string")
            return internalBase64EncodeURL(value);
        throw new Error('Invalid type passed to Base64.encodeURL.');
    };


    /** Decode a base 64 string. This assumes standard base 64
     *  encoding, so no special decoding of escaped characters is
     *  performed.
     *  @param {string} value a string or array of data to encode. It will use utf-8 encoding.
     *  @returns {string}
     *  @throws {Error} if the parameter is invalid (empty or not a valid type).
     */
    util.Base64.decode = function(value) {
        if (typeof(value) === "string")
            return internalBase64Decode(value);
        throw new Error('Invalid type passed to Base64.decode.');
    };

    /** Decode a base 64 string. This assumes standard base 64
     *  encoding, so no special decoding of escaped characters is
     *  performed.
     *  @param {string} value a string or array of data to encode
     *  @returns {string} base 64 encoded data
     *  @throws {Error} if the parameter is invalid (empty or not a valid type).
     */
    util.Base64.decodeURL = function(value) {
        if (typeof(value) === "string")
            return internalBase64DecodeURL(value);
        throw new Error('Invalid type passed to Base64.decodeURL.');
    };

})();
