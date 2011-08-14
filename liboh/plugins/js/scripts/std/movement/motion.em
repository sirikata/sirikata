/*  Sirikata
 *  motion.em
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

system.require('std/core/repeatingTimer.em');
system.require('std/core/pretty.em');

system.require('units.em');

if(typeof(motion) === 'undefined')
    /**
     * @namespace
     * An object motion controller library<br /><br />
     *
     * Defines a set of classes that continuously monitor a presence's state and
     * change its position or velocity when necessary.  To assign a motion
     * controller to a presence, simply create a controller with the presence as
     * the first parameter to the constructor:<br /><code>
     *        var controller = new motion.SomeType(presence, options...);</code>
     */
	motion = {};
if(typeof(motion.util) === 'undefined')
    /** @namespace */
	motion.util = {};

motion.util.isVector = function(obj) {
	return (typeof(obj) !== 'undefined' &&
			'__getType' in obj && obj.__getType() == 'vec3');
};

motion.util.isQuat = function(obj) {
	return (typeof(obj) !== 'undefined' &&
			'__getType' in obj && obj.__getType() == 'quat');
};

motion.util.isVisible = function(obj) {
    return (typeof(obj) !== 'undefined' && '__getType' in obj &&
            (obj.__getType() == 'presence' || obj.__getType() == 'visible'));
};

/**
 * @param {object} pres
 * @return {number} the mass of an object, usually a presence or visible, as it is
 *      interpreted by the motion library.  First attempts to retrieve the mass
 *      from its physics properties, then looks for a 'mass' field of the
 *      object.  If neither of these are present, assigns a default value of
 *      1 kg.
 */
motion.util.mass = function(pres) {
    if('mass' in pres)
        return pres.mass;
    else if('physics' in pres && 'mass' in pres.physics)
        return pres.physics.mass;
    else
        return 1 * u.kg;
};

/**
 * @param {object} pres
 * @return {number} the moment of inertia of an object, usually a presence or visible,
 *      as it it interpreted by the motion library.  Looks for an 'inertia'
 *      field of the object; if one isn't present, assigns a default value of
 *      2/5 * mass * scale^2 (as if it were a solid sphere).
 */
motion.util.inertia = function(pres) {
    if('inertia' in pres)
        return pres.inertia;
        // TODO: how is moment of inertia tensor handled in Bullet interface?
    else
        // treat as sphere by default
        return .4 * motion.util.mass(pres) * pres.scale * pres.scale;
};

/**
 * Motion controllers operate on repeating timers.  The last (optional)
 * argument to every class of controller specifies the repeat period at
 * which the timer operates; assigning longer periods means less network
 * traffic but a slower update rate.  defaultPeriod is the period for all
 * controllers whose last argument is not given.
 * @constant
 */
motion.defaultPeriod = 0.06 * u.s;


/**
 * @class Base class for all motion controllers.  This sets up the basic
 * repeating timer code, with a callback specified by base classes.  Generally
 * intended as an abstract base class, but could be useful on its own with a
 * specialized callback.  The core methods available for all controllers are
 * also defined here.
 *
 * @param {presence} presence The presence to control.  This may be changed
 *        later by assigning to <code>controller.presence</code>.
 * @param {function(presence)->undefined} fn The callback function to call
 *      repeatedly, with the presence as a parameter
 * @param {number} period (optional =defaultPeriod) The period at which the
 *        callback is called
 */
motion.Motion = system.Class.extend(/** @lends motion.Motion# */{
	init: function(presence, fn, period) {
		if(typeof(period) === 'undefined')
			period = motion.defaultPeriod;

		var self = this; // so callbacks can refer to instance variables
		
		self.period = period;
        /**
         * The presence the motion controller controls.
	 * @exports self.presence as motion.Motion#presence
         * @type presence
         */
		self.presence = presence;
		self.timer = new std.core.RepeatingTimer(self.period, function() {
			fn(self.presence);
		});
	},
	
	/**
	 * Pauses the operation of the controller.  The controller can be resumed
	 * at any time by calling <code>reset</code>.
	 */
	suspend: function() {
		this.timer.suspend();
	},
	
	/**
	 * Restarts the controller, resuming if suspended.
	 */
	reset: function() {
		this.timer.reset();
	},
	
	/**
     * @return {boolean} <code>true</code> if the controller is currently suspended,
	 *		<code>false</code> otherwise.
	 */
	isSuspended: function () {
		return this.timer.isSuspended();
	}
});

/**
 * @class A controller for applying accelerations to an object.
 *
 * @param {presence} presence The presence to control
 * @param {function(presence)->(util.Vec3|undefined)} accelFn (optional =get
 *        from presence.accel field) A function that should return the
 *        acceleration on a presence at any point in time.  If
 *		accelFn returns undefined ("return;"), the acceleration will be
 *		unchanged from the last call.  If accelFn itself is undefined (or not
 *		provided), the controller will use the value of presence.accel.
 * @param {number} period (optional =defaultPeriod) The period at which the
 *		acceleration is updated
 * @augments motion.Motion
 */
motion.Acceleration = motion.Motion.extend(/** @lends motion.Acceleration# */{
	init: function(presence, accelFn, period) {
		var self = this;
		if(typeof(presence.accel) === 'undefined')
			presence.accel = <0, 0, 0>;
		if(typeof(accelFn) === 'undefined')
			accelFn = function(p) { return p.accel; };
		else if(typeof(accelFn) !== 'function')
			throw new Error('second argument "accelFn" to motion.Acceleration (' +
					std.core.pretty(accelFn) +
					' is not a function or undefined');
		
		var callback = function(p) {
			var accel = accelFn(p);
            if(motion.util.isVector(accel))
				p.accel = accel;
			else if(typeof(accel) != 'undefined')
				throw new Error('in motion.Acceleration callback: accelFn should return ' +
						'a vector or undefined (instead got ' +
						std.core.pretty(accel) + ')');
			
			// we need to apply a change in velocity directly, since
			// acceleration is not a core feature
			p.velocity = p.velocity + p.accel.scale(self.period);
		};
		this._super(presence, callback, period);
	}
});

/**
 * @class A controller for manipulating the velocity of a presence.
 * @param {presence} presence The presence to control
 * @param {function(presence)->(util.Vec3|undefined)} velFn A function that
 *         should return the new velocity of the presence
 * 		at any point in time (just return; to leave velocity unchanged)
 * @param {number} period (optional =defaultPeriod) The period at which the presence's
 *		velocity is updated
 * @augments motion.Motion
 */
motion.Velocity = motion.Motion.extend(/** @lends motion.Velocity# */{
	init: function(presence, velFn, period) {
		if(typeof(velFn) !== 'function')
			throw new Error('second argument "velFn" to motion.Velocity (' +
					std.core.pretty(velFn) + ') is not a function');
		
		var callback = function(p) {
			var vel = velFn(p);
            if(motion.util.isVector(vel))
				p.velocity = vel;
			else if(typeof(vel) != 'undefined')
				throw new Error('in motion.Velocity callback: velFn should return ' +
						'a vector or undefined (instead got ' +
						std.core.pretty(vel) + ')');
		};
		this._super(presence, callback, period);
	}
});

/**
 * @class A controller for manipulating the position of a presence directly.
 * Note that if this is used for constant updates, the position changes will
 * appear abrupt and jittery -- this type of controller is best used for
 * sudden, infrequent changes (such as teleportation).
 *
 * @param {presence} presence The presence to control
 * @param {function(presence)->(util.Vec3|undefined)} posFn A function that
 *         should return the new position of the presence
 * 		at any point in time (just return; to leave position unchanged)
 * @param {number} period (optional =defaultPeriod) The period at which the
 *        presence's position is updated
 * @augments motion.Motion
 */
motion.Position = motion.Motion.extend(/** @lends motion.Position# */{
	init: function(presence, posFn, period) {
		if(typeof(posFn) !== 'function')
			throw new Error('second argument "posFn" to motion.Position (' +
					std.core.pretty(posFn) + ') is not a function');
		
		var callback = function(p) {
			var pos = posFn(p);
            if(motion.util.isVector(pos))
				p.position = pos;
			else if(typeof(pos) != 'undefined')
				throw new Error('in motion.Position callback: posFn should return ' +
						'a vector or undefined (instead got ' +
						std.core.pretty(pos) + ')');
		};
		this._super(presence, callback, period);
	}
});

/**
 * @class A controller for manipulating the orientation of a presence.  This
 *		is best used for infrequent updates or in combination with an
 *		OrientationVel controller; frequent raw orientation updates will
 *		appear jittery.
 * @param {presence} presence The presence to control
 * @param {function(presence)->(util.Quaternion|undefined)} orientFn A function
 *        that should return the new orientation of the presence at any point
 *        in time (just return; to leave orientation unchanged)
 * @param {number} period (optional =defaultPeriod) The period at which the 
 *        presence's orientation is updated
 * @augments motion.Motion
 */
motion.Orientation = motion.Motion.extend(/** @lends motion.Orientation# */{
	init: function(presence, orientFn, period) {
		if(typeof(orientFn) !== 'function')
			throw new Error('second argument "orientFn" to motion.Orientation (' +
					std.core.pretty(orientFn) + ') is not a function');
		
		var callback = function(p) {
			var orient = orientFn(p);
            if(motion.util.isQuat(orient))
				p.orientation = orient;
			else if(typeof(orient) != 'undefined')
				throw new Error('in motion.Orientation callback: orientFn should return ' +
						'a quaternion or undefined (instead got ' +
						std.core.pretty(orient) + ')');
		};
		this._super(presence, callback, period);
	}
});

/**
 * @class A controller for manipulating the orientation velocity of a
 * 		presence.
 * @param {presence} presence The presence to control
 * @param {function(presence)->(util.Quaternion|undefined)} oVelFn A function
 *         that should return the new orientation velocity of the presence at
 *         any point in time (just return; to leave orientation unchanged)
 * @param {number} period (optional =defaultPeriod) The period at which the
 *         presence's orientation velocity is updated
 */
motion.OrientationVel = motion.Motion.extend(/** @lends motion.OrientationVel# */{
	init: function(presence, oVelFn, period) {
		if(typeof(oVelFn) !== 'function')
			throw new Error('second argument "oVelFn" to motion.OrientationVel (' +
					std.core.pretty(oVelFn) + ') is not a function');
		
		var callback = function(p) {
			var oVel = oVelFn(p);
            if(motion.util.isQuat(oVel))
				p.orientationVel = oVel;
			else if(typeof(oVel) != 'undefined')
				throw new Error('in motion.Orientation callback: oVelFn should return ' +
						'a quaternion or undefined (instead got ' +
						std.core.pretty(oVel) + ')');
		};
		this._super(presence, callback, period);
	}
});

/**
 * @class A controller that applies angular acceleration to a presence.
 * @param {presence} presence The presence to control
 * @param {function(presence)->(util.Vec3|util.Quaternion|undefined)} oAccelFn
 *      A function that should return the angular acceleration on the presence
 *      at any point in time, as a vector or a quaternion.  If oAccelFn returns
 *      undefined ("return;"), the acceleration will be unchanged from the last
 *      call.  If accelFn itself is undefined (or not provided), the controller
 *      will use the value of presence.orientationAccel, which if assigned
 *      externally, should be a vector (axis with magnitude in rads/sec^2).
 * @param {number} period (optional =defaultPeriod) The period at which the
 *		angular acceleration is updated
 * @augments motion.Motion
 */
motion.OrientationAccel = motion.Motion.extend(/** @lends motion.OrientationAccel# */{
	init: function(presence, oAccelFn, period) {
		var self = this;
		if(typeof(presence.orientationAccel) === 'undefined')
			presence.orientationAccel = <0, 0, 0>;
		if(typeof(oAccelFn) === 'undefined')
			oAccelFn = function(p) { return p.orientationAccel; };
		else if(typeof(oAccelFn) !== 'function')
			throw new Error('second argument "oAccelFn" to motion.OrientationAccel (' +
					std.core.pretty(oAccelFn) +
					' is not a function or undefined');
		
		var callback = function(p) {
			var oAccel = oAccelFn(p);
            if(motion.util.isVector(oAccel))
				p.orientationAccel = oAccel;
            else if(motion.util.isQuat(oAccel))
                p.orientationAccel = oAccel.axis().scale(oAccel.length());
			else if(typeof(oAccel) != 'undefined')
				throw new Error('in motion.Acceleration callback: oAccelFn should return ' +
						'a vector, a quaternion or undefined (instead got ' +
						std.core.pretty(oAccel) + ')');
			
            var angVel = p.orientationVel.axis().scale(p.orientationVel.length());
			angVel = angVel + p.orientationAccel.scale(self.period);
            var mag = angVel.length();
            if(mag < 1e-8)
                p.orientationVel = new util.Quaternion();
            else
                p.orientationVel = (new util.Quaternion(angVel.div(mag),
                        1)).scale(mag);
		};
		this._super(presence, callback, period);
	}
});

/**
 * The default acceleration of an object under a Gravity controller.
 * @constant
 */
motion.defaultGravity = 9.80665 * u.m / u.s / u.s;

/**
 * @class Accelerates a presence downward under a constant gravitational force.
 *
 * @param {presence} presence The presence to accelerate
 * @param {util.Vec3|number} accel (optional =<0, -defaultGravity, 0>) The
 *        acceleration of gravity (as either a scalar quantity or a vector).
 *        This may be changed later through <code>controller.accel</code>, but
 *        only as a vector.
 * @param {number} period (optional =defaultPeriod) The period at which the
 *        presence's velocity is updated
 * @augments motion.Acceleration
 */
motion.Gravity = motion.Acceleration.extend(/** @lends motion.Gravity# */{
	init: function(presence, accel, period) {
		var self = this;
		
		if(typeof(accel) === 'number')
            /**
             * The acceleration of this gravity controller.  Can be
             * modified dynamically.
             * @exports self.accel as motion.Gravity#accel
             * @type util.Vec3
             */
			self.accel = <0, -accel, 0>;
		else
			self.accel = accel || <0, -motion.defaultGravity, 0>;
		
		this._super(presence, function() { return self.accel; }, period);
	}
});

/**
 * @class Accelerates a presence under a harmonic spring force.
 *
 * @param {presence} presence The presence to control
 * @param {util.Vec3|visible|presence} anchor The anchor point around which the
 *        presence oscillates.  This can be a vector (point in space) or
 *        another presence or visible (which will be examined as its position
 *        changes).  It can be changed later through
 *        <code>controller.anchor</code>.
 * @param {number} stiffness The stiffness or "spring constant" of the spring
 *        force -- the greater the stiffness, the greater the force at the same
 *        distance
 * @param {number} damping (optional =0) The damping or "friction" of the
 *        spring motion
 * @param {number} eqLength (optional =0) The equilibrium length of the spring;
 *        if positive, the presence will be accelerated *away* from the anchor
 *		point if it gets too close
 * @param {number} period (optional =defaultPeriod) The period at which the
 *        presence's velocity is updated
 * @augments motion.Acceleration
 */
motion.Spring = motion.Acceleration.extend(/** @lends motion.Spring# */{
	init: function(presence, anchor, stiffness, damping, eqLength, period) {
		var self = this;
		
        /**
         * The stiffness (spring constant, units force/length) of the spring.
	 * @exports self.stiffness as motion.Spring#stiffness
	 * @type number
         */
		self.stiffness = stiffness;
	/**
         * The equilibrium length of the spring (units length).
         * @exports self.eqLength as motion.Spring#eqLength
         * @type number
         */
		self.eqLength = eqLength || 0;
	/**
         * The damping constant (units force/speed) of the spring.
         * @exports self.damping motion.Spring#damping
         * @type number
         */
		self.damping = damping || 0;
		
	/**
         * The anchor position of the spring, or an object with a 'position'
         * field that the spring is anchored to.
         * @exports self.anchor as motion.Spring#anchor
         * @type util.Vec3|presence|visible|object
         */
		self.anchor = anchor;
        var anchorFn = function() {
		if(typeof(self.anchor) === 'object' && 'x' in self.anchor)
		return self.anchor;
		else if(typeof(anchor) === 'object' && 'position' in self.anchor)
		return self.anchor.position;
		else
		throw new Error("Field 'anchor' of motion.Spring object ('" +
				std.core.pretty(self.anchor) +
					"') is not a vector or presence");
	}
		
		var accelFn = function(p) {
            var mass = motion.util.mass(p);
		
			var disp = (p.position - anchorFn());
			var len = disp.length();
			if(len < 1e-08)
				// even if eqLength is nonzero, we don't know which way to push
				// the object if it's directly on top of the anchor.
				return <0, 0, 0>;
			
			return disp.scale((self.stiffness * (self.eqLength - len) -
					self.damping * p.velocity.dot(disp)) / (len * mass));
		};
		
		this._super(presence, accelFn, period);
	}
});

motion._allCollisions = [];

/**
 * @class A generic controller to detect and respond to collisions.
 * 		All arguments except <code>period</code> can be modified later through
 *		fields of the same name (e.g. controller.testFn).
 *
 * @param {presence} presence The presence whose collisions are to be detected
 *        (the "colliding presence")
 * @param {function(presence)->object} testFn A function that should detect any
 *        collisions when called repeatedly and return one in the form of a
 *        "collision object"
 * @param {function(presence, object)->undefined} responseFn A function to be
 *        called when a collision happens
 * @param {number} period (optional =defaultPeriod) The period at which to
 *        check for collisions
 * @augments motion.Motion
 */
motion.Collision = motion.Motion.extend(/** @lends motion.Collision# */{
	init: function(presence, testFn, responseFn, period) {
		var self = this;
		
	/**
         * The function that the collision controller calls to test for
	 * collisions every tick.  May be safely replaced dynamically.
	 * @function
         * @param {presence} pres The presence to test for collisions
         * @return {object} A collision object (see
         *      <a href="http://sirikata.com/wiki/index.php?title=User:Wmonroe4/Motion_controllers">the
         *      Sirikata wiki tutorial</a> for the structure of collision objects)
         * @exports self.testFn as motion.Collision#testFn
         * @type object
         */
		self.testFn = testFn;

	/**
         * The function that the collision controller calls when a collision
         * occurs.  May be safely replaced dynamically.
	 * @function
         * @param {presence} pres The presence that is colliding
         * @param {object} collision A collision object (see
         *      <a href="http://sirikata.com/wiki/index.php?title=User:Wmonroe4/Motion_controllers">the
         *      Sirikata wiki tutorial</a> for the structure of collision objects)
         * @exports self.responseFn as motion.Collision#responseFn
         */
		self.responseFn = responseFn;
		
        var onCollisionMessage = function(message, sender) {
            // make sure that collision is always from the receiving object's
            // perspective
            if(message.collision.other.id === presence.toString()) {
                var temp = message.collision.self;
                message.collision.other = message.collision.self;
                message.collision.self = temp;
                message.collision.normal = message.collision.normal.neg();
            }

            motion._allCollisions.push(message.collision);

            self.responseFn(presence, message.collision);
        };

        onCollisionMessage << [{'action':'collision':},
                {'id':presence.toString():},
			       {'collision'::}];
		
        if(!('handlers' in motion.Collision))
            motion.Collision.handlers = {};
        if(!(presence.toString() in motion.Collision.handlers))
            motion.Collision.handlers[presence.toString()] = [];
        motion.Collision.handlers[presence.toString()].
                         push(onCollisionMessage);

        var sendCollisionEvent = function(msg, id) {
            if(id in motion.Collision.handlers) {
                for(var i in motion.Collision.handlers[id])
                    motion.Collision.handlers[id][i](msg);
            } else {
                msg >> system.createVisible(id) >> [];
            }
        };


		var testCollision = function(p) {
				var collision = self.testFn(p);
            if(collision) {
                sendCollisionEvent({
                    action: 'collision',
                    id: collision.self.id,
                    collision: collision
                }, collision.self.id);

                if(typeof(collision.other.id) === 'string') {
                    sendCollisionEvent({
                        action: 'collision',
                        id: collision.other.id,
                        collision: collision
                    }, collision.other.id);
				}
			}
		};
		
		this._super(presence, testCollision, period);
	},

    suspend: function() {
        this.collisionHandler.suspend();
        this._super();
    },

    reset: function() {
        this.collisionHandler.reset();
        this._super();
	}
});

/**
 * The default vector to use as "up" in making a presence look forward.
 * @constant
 */
motion.defaultUp = <0, 1, 0>;

/**
 * @class A controller that always points an object in the direction it is
 *		currently moving.
 *
 * @param {presence} presence The presence to control.
 * @param {util.Vec3} up (optional =defaultUp) The direction that the presence
 *         will use to orient itself so it is right-side up in addition to
 *         facing forward
 * @param {number} period (optional =defaultPeriod) The period at which to
 *         update the presence's orientation.
 * @augments motion.Orientation
 */
motion.LookForward = motion.Orientation.extend(/** @lends motion.LookForward# */{
	init: function(presence, up, period) {
	var self = this;
	/**
         * The vector to use as up in calculating the forward orientation.
         * @exports self.up as motion.LookForward#up
         * @type util.Vec3
         */
        self.up = up || motion.defaultUp;
		
		var useAccel = function(p) {
			if(!('accel' in p))
				return;
			
			var omega = p.velocity.cross(p.accel).
					div(p.velocity.lengthSquared());
                    // TODO: the model orientation adjustment should really be in
                    // std/shim/presence.em -- one shouldn't need to take into account
                    // model orientation when setting ovel
                    return <p.modelOrientation.inv() * p.orientation.inv() *
                        omega.normal(); 1> * omega.length();
		};
		this.oVelController = new motion.OrientationVel(presence, useAccel,
				period);
		
		var lookForward = function(p) {
			if(p.velocity.length() < 1e-8)
				return;
                    return util.Quaternion.fromLookAt(p.velocity, self.up);
		};
		this._super(presence, lookForward, period);
	}
});

/**
 * @class A controller that applies force and optionally torque to a presence.
 * @param {presence} presence The presence to control
 * @param {util.Vec3|function(presence)->util.Vec3} force The force to apply
 * @param {util.Vec3|presence|visible|function(presence)->util.Vec3} position
 *      (optional =presence's position) The position (in world
 *      coordinates) at which to apply the force, as a vector, a visible whose
 *      position is tracked, or a function that returns a vector when called.
 *      If position is undefined or not given, the position of the presence is
 *      used (and therefore no torque is exerted).
 * @param {number} period The period at which to change the presence's
 *      velocities when applying force and torque.
 * @augments motion.Acceleration
 */
motion.ForceTorque = motion.Acceleration.extend(/** @lends motion.ForceTorque# */{
    init: function(presence, force, position, period) {
        var forceFn;
        if(typeof(force) === 'function')
            forceFn = force;
        else if(motion.util.isVec(force))
            forceFn = function(p) { return force; };
        else
			throw new Error("Second argument 'force' to motion.ForceTorque constructor ('" +
					std.core.pretty(force) +
					"') is not a function or vector");

        var posFn;
        if(typeof(position) === 'function')
        /** @ignore */
            posFn = position;
        else if(typeof(position) === 'undefined')
        /** @ignore */
            posFn = function(p) { return p.position; };
        else if(motion.util.isVec(position))
            posFn = function(p) { return position; };
        else if(motion.util.isVisible(position))
            posFn = function(p) { return position.position; };
        else
			throw new Error("Second argument 'position' to motion.ForceTorque constructor ('" +
					std.core.pretty(position) +
					"') is not a vector, visible, function, or undefined");
        
        var accelFn = function(p) {
            return <0, 0, 0>;
            // return forceFn(p).div(motion.util.mass(p));
        };
        var oAccelFn = function(p) {
            return p.orientation.inverse().mul((posFn(p) -
                    p.position).cross(forceFn(p))).div(motion.util.inertia(p));
        };

        this.oAccel = new motion.OrientationAccel(presence, oAccelFn, period);
        this._super(presence, accelFn, period);
    }
});
