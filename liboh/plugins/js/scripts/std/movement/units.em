/* units.em
 *
 * Variables to define an explicit system of units
 * 
 * These variables (in the module u) define a system of units to use in
 * Sirikata.
 *
 * Inherently, the choice of variables assigned to the value 1 defines the
 * a set of conventions for interacting with modules that do not adhere to
 * this unit system.  For example, if the documentation of a third-party
 * module says "ThirdPartyTime() returns the time in milliseconds since the
 * party started", the right way to store the return value using this units
 * module would be:
 *var clock = ThirdPartyTime() * u.ms;
 *
 * I chose SI/standard scientific units as the basis for this module; most
 * units that I have defined to be numerically equal to 1 are from this
 * family.
 *
 * Note that there is no type-safety or other kind of safety associated with
 * using these units, only readability and convenience.  They are just
 * numbers, so if you don't pick the right units, you'll just get the wrong
 * numbers.  (Of course, if you're using mostly standard units, a lot of them
 * will be 1, so you might get lucky.  Still, watch your multiplication and
 * division!)
 */


if(typeof(u) === 'undefined')
    u = {};

/**
 * @function convert(quantity, targetUnits)
 * Use to pass a value in this unit system to a module expecting a different
 * system of units.
 * @return the value quantity in the units given by targetUnits
 */
u.convert = function(quantity, targetUnits) {
    return quantity / targetUnits;
};

/* ===Standard SI prefixes===
 *
 * Multiply these by any unit to derive another unit.  (Watch your order of
 * operations!)
 *
 * If you need prefixes outside of this range (exa, femto, etc.), you may want
 * to restructure your measurement system for numerical stability reasons.
 */
u.micro = 1e-6;
u.milli = 0.001;
u.centi = 0.01;
u.deci = 0.1;
u.deka = 10;
u.hecto = 100;
u.kilo = 1000;
u.mega = 1e6;

/**
 * @constant
 * Standard units of time.
 */
u.second = (u.s = 1);

u.millisecond = (u.ms = u.milli * u.second);

u.minute = (u.min = 60 * u.second);
u.hour = (u.hr = 60 * u.minute);
u.day = 24 * u.hour;
u.week = 7 * u.day;
u.year = 365.25 * u.day;

/**
 * @constant
 * Standard units of length.
 */
u.meter = (u.m= 1);

u.millimeter = (u.mm = u.milli * u.meter);
u.centimeter = (u.cm = u.centi * u.meter);
u.kilometer = (u.km = u.kilo * u.meter);

u.foot = (u.ft = 0.3048 * u.meter);
u.inch = u.foot / 12;
u.mile = 5280 * u.foot;

/**
 * @constant
 * Standard units of angle measure.
 */
u.radian = (u.rad = 1);

u.turn = 2 * Math.PI * u.radian;
u.degree = (u.deg = u.turn / 360);

/**
 * @constant
 * Standard units of solid angle measure.
 */
u.steradian = (u.sr = 1);

u.sphere = 4 * Math.PI * u.steradian;
u.squareDegree = u.degree * u.degree;

/**
 * @constant
 * Standard units of mass.
 */
u.kilogram = (u.kg = 1);

u.gram = (u.g = 0.001 * u.kilogram);
// SI's a bit weird here.  Who am I to go against convention, though?

u.pound = (u.lb = 0.45359237 * u.kilogram);
u.ounce = (u.oz = u.pound / 16);
u.ton = 2000 * u.pound;

/**
 * @constant
 * Standard units of force.
 */
u.newton = (u.N = u.kilogram * u.meter / (u.second * u.second));
// Scan the units above and you'll see that u.N == 1.  Better to define
// as much as possible in terms of existing units, though.
u.poundForce = (u.lbf = u.pound * 9.80665 * u.meter /
                (u.second * u.second));
