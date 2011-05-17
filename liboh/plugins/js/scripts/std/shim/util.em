
{
  /** @namespace visible */
  var util = function()
  {

      /**
       Overloads the '-' operator for many types.  a and b must be of the same type
       (either vectors or numbers).  If a and b are vectors (a =
       <ax,ay,az>; b = <bx,by,bz>, returns <ax-bx, ay-by, az-bz>).  If a and b are
       numbers, returns a - b.

       @param a Of type vector or number.
       @param b Of type vector or number.

       @return vector or number
       */
      util.prototype.minus = function()
      {
      };


      /**
       Overloads the '+' operator for many types.  a and b must be of the same type
       (either vectors, numbers, or strings).  If a and b are vectors (a =
       <ax,ay,az>; b = <bx,by,bz>, returns <ax+bx, ay+by, az+bz>).  If a and b are
       numbers, returns a + b.  If a and b are strings, returns concatenated string.
       
       @param a Of type vector, number, or string.
       @param b Of type vector, number, or string.
       @return vector, number, or string
       */
      util.prototype.plus = function()
      {
      };


      /**
       @return a random float from 0 to 1
       */
      util.prototype.rand = function()
      { };

      /**
       @param takes in a single argument
       @return returns a float
       */
      util.prototype.sqrt = function()
      {
      };


      /**
       @param float to take arccosine of
       @return angle in radians
       */
      util.prototype.acos = function()
      {
          
      };

      /**
       @param angle in radians to take cosine of
       @return cosine of that angle
       */
      util.prototype.cos = function()
      {
          
      };


      /**
       @param angle in radians to take sine of
       @return sine of that angle
       */
      util.prototype.sin = function()
      {
          
      };


      /**
       @param float to take arcsine of
       @return angle in radians
       */
      util.prototype.asin = function()
      {
          
      };


      /**
       @param base
       @param exponent
       @return returns base to the exponent
       */
      util.prototype.pow = function()
      {
          
      };


      /**
       @param exponent
       @return returns e to the exponent
       */
      util.prototype.exp = function()
      {
      };

      /**
       @param number to take abs of
       @return returns absolute value of argument.
       */
      util.prototype.abs = function()
      {
          
      };


      /**
       @param name to match
       @param value to match (can be null)
       @param proto to match (can be null)

       @return pattern object
       */
      util.prototype.Pattern =function()
      {
          
      };

      /**
       @param x
       @param y
       @param z
       @param w

       @return quaternion object
       
       */
      util.prototype.Quaternion = function()
      {
          
      };

      /**
       @param x
       @param y
       @param z

       @return a vec3 object
       */
      util.prototype.Vec3 = function()
      {
      };
  };
}