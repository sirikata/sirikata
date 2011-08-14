/*  Sirikata
 *  units.em
 *
 *  Copyright (c) 2011, William Monroe
 *  All rights reserved.
 * 
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Sirikata nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

if(typeof(u) === 'undefined')
    /**
     * @namespace
     *
     * Variables to define an explicit system of units<br /><br />
     * 
     * These variables (in the module <code>u</code>) define a system of units
     * to use in Sirikata.<br /><br />
 *
 * Inherently, the choice of variables assigned to the value 1 defines the
 * a set of conventions for interacting with modules that do not adhere to
 * this unit system.  For example, if the documentation of a third-party
     * module says "<code>ThirdPartyTime()</code> returns the time in
     * milliseconds since the party started", the right way to store the return
     * value using this units module would be:<br /><code>
     *      var clock = ThirdPartyTime() * u.ms;</code><br /><br />
 *
 * I chose SI/standard scientific units as the basis for this module; most
 * units that I have defined to be numerically equal to 1 are from this
     * family.<br /><br />
 *
 * Note that there is no type-safety or other kind of safety associated with
 * using these units, only readability and convenience.  They are just
 * numbers, so if you don't pick the right units, you'll just get the wrong
 * numbers.  (Of course, if you're using mostly standard units, a lot of them
 * will be 1, so you might get lucky.  Still, watch your multiplication and
 * division!)
 */
    u = {};

/**
 * Use to pass a value in this unit system to a module expecting a different
 * system of units.  Example:
 * @example >>> u.convert(42 * u.year, u.week)
 * 2191.5
 * @param {number} quantity A quantity in this standard units system; e.g.
 *      <code>4 * u.foot</code>
 * @param {number} targetUnits The units in which to report
 *      <code>quantity</code>
 * @return the numerical value of <code>quantity</code> in the system of units
 *      given by <code>targetUnits</code>
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
/** @constant */
u.micro = 1e-6;
/** @constant */
u.milli = 0.001;
/** @constant */
u.centi = 0.01;
/** @constant */
u.deci = 0.1;
/** @constant */
u.deka = 10;
/** @constant */
u.hecto = 100;
/** @constant */
u.kilo = 1000;
/** @constant */
u.mega = 1e6;

/**
 * Standard unit of time.
 * @constant
 */
u.second = 1;
    /**
     * =u.second
 * @constant
 */
    u.s = u.second;

    /** @constant */
    u.millisecond = u.milli * u.second;
        /**
	 * =u.millisecond
	 * @constant
	 */
        u.ms = u.millisecond;

    /** @constant */
    u.minute = 60 * u.second;
    /**
     * =u.minute
     * @constant
     */
    u.min = u.minute;
    /** @constant */
    u.hour = 60 * u.minute;
        /**
         * =u.hour
         * @constant
         */
        u.hr = u.hour;
    /** @constant */
    u.day = 24 * u.hour;
    /** @constant */
    u.week = 7 * u.day;
    /** @constant */
    u.year = 365.25 * u.day;

/**
 * Standard unit of length.
 * @constant
 */
u.meter = (u.m= 1);

    /** @constant */
    u.millimeter = u.milli * u.meter;
        /**
         * =u.millimeter
         * @constant
         */
        u.mm = u.millimeter;
    /** @constant */
    u.centimeter = u.centi * u.meter;
        /**
         * =u.centimeter
         * @constant
         */
        u.cm = u.centimeter;
    /** @constant */
    u.kilometer = u.kilo * u.meter;
        /**
         * =u.kilometer
         * @constant
         */
        u.km = u.kilometer;

/** @constant */
u.foot = 0.3048 * u.meter;
    /**
     * =u.foot
     * @constant
     */
    u.ft = u.foot;
    /** @constant */
    u.inch = u.foot / 12;
    /** @constant */
    u.mile = 5280 * u.foot;

/**
 * Standard unit of angle measure.
 * @constant
 */
u.radian = 1;
    /** @constant */
    u.rad = u.radian;

    /** @constant */
    u.revolution = 2 * Math.PI * u.radian;
        /**
         * =u.revolution
         * @constant
         */
        u.rev = u.revolution;
    /** @constant */
    u.degree = u.revolution / 360;
        /**
         * =u.degree
         * @constant
         */
        u.deg = u.degree;

/** @constant */
u.rpm = u.revolution / u.minute;

/**
 * Standard unit of solid angle measure.
 * @constant
 */
u.steradian = 1;
    /**
     * =u.steradian
 * @constant
 */
    u.sr = u.steradian;

    /** @constant */
    u.sphere = 4 * Math.PI * u.steradian;
    /** @constant */
    u.squareDegree = u.degree * u.degree;

/**
 * Standard unit of mass.
 * @constant
 */
u.kilogram = 1;
    /**
     * =u.kilogram
 * @constant
 */
    u.kg = u.kilogram;

    /** @constant */
    u.gram = 0.001 * u.kilogram;
    // SI's a bit weird here.  Who am I to go against convention, though?
        /**
         * =u.gram
         * @constant
         */
        u.g = u.gram;

/** @constant */
u.pound = 0.45359237 * u.kilogram;
    /**
     * =u.pound
     * @constant
     */
    u.lb = u.pound;
    /** @constant */
    u.ounce = u.pound / 16;
        /**
         * =u.ounce
         * @constant
         */
        u.oz = u.ounce;
    /** @constant */
    u.ton = 2000 * u.pound;

/**
 * Standard unit of force.
 * @constant
 */
u.newton = u.kilogram * u.meter / (u.second * u.second);
// Scan the units above and you'll see that u.N == 1.  Better to define
// as much as possible in terms of existing units, though.
    /**
     * =u.newton
     * @constant
     */
    u.N = u.newton;

    /** @constant */
    u.poundForce = u.pound * 9.80665 * u.meter / (u.second * u.second);
        /**
         * =u.poundForce
         * @constant
         */
        u.lbf = u.poundForce;
