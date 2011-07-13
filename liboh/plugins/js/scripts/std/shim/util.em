
(function()
{
    
    var internalUtilPlus = std.core.bind(util.plus,util);
    var internalUtilSub  = std.core.bind(util.sub,util);
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

    // var internalUtilPattern = std.core.bind(util.Pattern,util);
    // var internalUtilVec3 = std.core.bind(util.Vec3,util);
    // var internalUtilQuaternion = std.core.bind(util.Quaternion,util);

    var internalUtilPattern = util.Pattern;
    var internalUtilVec3 = util.Vec3;
    var internalUtilQuaternion = util.Quaternion;
    
    
    /**
     Overloads the '+' operator.

     Checks if lhs has a plus function defined on it.  If it does, then
     calls lhs.plus(rhs).  If it doesn't, call internal util
     addition.  Internal requires that both lhs and rhs are of same
     type.  and can either be a vector, number, quaternion, or string.

     */
    util.plus = function (lhs, rhs)
    {
        if (typeof(lhs.add) == 'function')
            return lhs.add(rhs);                

        return internalUtilPlus.apply(undefined,arguments);
    };


    /**
     Overloads the '-' operator.
     
     Checks if lhs has a sub function defined on it.  If it does, then
     calls lhs.sub(rhs).  If it doesn't, call internal util
     subtraction.  Internal requires that both lhs and rhs are of same
     type.  and can either be a vector, number, or quaternion.
     */
    util.sub = function (lhs,rhs)
    {
        if (typeof(lhs.sub) == 'function')
            return lhs.sub(rhs);
        
        return internalUtilSub.apply(undefined,arguments);
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
          return internalUtilIdentifer.apply(undefined,arguments);
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
       @param x
       @param y
       @param z
       @param w

       @return quaternion object
       
       */
      util.Quaternion = function()
      {
          return internalUtilQuaternion.apply(this,arguments);
      };

      /**
       @param x
       @param y
       @param z

       @return a vec3 object
       */
      util.Vec3 = function()
      {
          return internalUtilVec3.apply(this,arguments);
      };
    
    
})();

