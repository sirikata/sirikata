/* motion.em
 *
 * An object motion controller library
 *
 * Author: Will Monroe
 *
 * Defines a set of classes that continuously monitor a presence's state and
 * change its position or velocity when necessary.  To assign a motion
 * controller to a presence, simply create a controller with the presence as
 * the first parameter to the constructor:
 *		var controller = new motion.SomeType(presence, options...);
 */

system.require('std/core/repeatingTimer.em');
system.require('std/core/pretty.em');

system.require('units.em');

if(typeof(motion) === 'undefined')
	motion = {};
if(typeof(motion.util) === 'undefined')
	motion.util = {};

motion.util._isVector = function(obj) {
	return (typeof(obj) !== 'undefined' &&
			'__getType' in obj && obj.__getType() == 'vec3');
};

motion.util._isQuat = function(obj) {
	return (typeof(obj) !== 'undefined' &&
			'__getType' in obj && obj.__getType() == 'quat');
};

motion.util._isVisible = function(obj) {
    return (typeof(obj) !== 'undefined' && '__getType' in obj &&
            (obj.__getType() == 'presence' || obj.__getType() == 'visible'));
};

motion.util._mass = function(pres) {
    if('mass' in pres)
        return pres.mass;
    else if('physics' in pres && 'mass' in pres.physics)
        return pres.physics.mass;
    else
        return 1 * u.kg;
};

motion.util._inertia = function(pres) {
    if('inertia' in pres)
        return pres.inertia;
        // TODO: how is moment of inertia tensor handled in Bullet interface?
    else
        // treat as sphere by default
        return .4 * motion.util._mass(pres) * pres.scale * pres.scale;
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
 * @param presence The presence to control.  This may be changed later by
 *		assigning to <code>controller.presence</code>.
 * @param fn The callback function to call repeatedly, with a presence as a
 * @param period (optional =defaultPeriod) The period at which the callback is
 *		called
 */
motion.Motion = system.Class.extend({
	init: function(presence, fn, period) {
		if(typeof(period) === 'undefined')
			period = motion.defaultPeriod;

		var self = this; // so callbacks can refer to instance variables
		
		self.period = period;
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
	 * @return <code>true</code> if the controller is currently suspended,
	 *		<code>false</code> otherwise.
	 */
	isSuspended: function () {
		return this.timer.isSuspended();
	}
});

/**
 * @class A controller for applying accelerations to an object.
 *
 * @param presence The presence to control
 * @param accelFn (optional =get from presence.accel field) A function that
 *		should return the acceleration on a presence at any point in time.  If
 *		accelFn returns undefined ("return;"), the acceleration will be
 *		unchanged from the last call.  If accelFn itself is undefined (or not
 *		provided), the controller will use the value of presence.accel.
 * @param period (optional =defaultPeriod) The period at which the
 *		acceleration is updated
 */
motion.Acceleration = motion.Motion.extend({
	init: function(presence, accelFn, period) {
		var self = this;
		if(typeof(presence.accel) === 'undefined')
			presence.accel = <0, 0, 0>;
		if(typeof(accelFn) === 'undefined')
			accelFn = function(p) { return p.accel; };
		else if(typeof(accelFn) !== 'function')
			throw('second argument "accelFn" to motion.Acceleration (' +
					system.core.pretty(accelFn) +
					' is not a function or undefined');
		
		var callback = function(p) {
			var accel = accelFn(p);
			if(motion.util._isVector(accel))
				p.accel = accel;
			else if(typeof(accel) != 'undefined')
				throw('in motion.Acceleration callback: accelFn should return ' +
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
 * @param presence The presence to control
 * @param posFn A function that should return the new velocity of the presence
 * 		at any point in time (just return; to leave velocity unchanged)
 * @param period (optional =defaultPeriod) The period at which the presence's
 *		velocity is updated
 */
motion.Velocity = motion.Motion.extend({
	init: function(presence, velFn, period) {
		if(typeof(accelFn) !== 'function')
			throw('second argument "velFn" to motion.Velocity (' +
					system.core.pretty(velFn) + ') is not a function');
		
		var callback = function(p) {
			var vel = velFn(p);
			if(motion.util._isVector(vel))
				p.velocity = vel;
			else if(typeof(vel) != 'undefined')
				throw('in motion.Velocity callback: velFn should return ' +
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
 * @param presence The presence to control
 * @param posFn A function that should return the new position of an object
 * 		at any point in time (just return; to leave position unchanged)
 * @param period (optional =defaultPeriod) The period at which the object's position is
 *		updated
 */
motion.Position = motion.Motion.extend({
	init: function(presence, posFn, period) {
		if(typeof(posFn) !== 'function')
			throw('second argument "posFn" to motion.Position (' +
					system.core.pretty(posFn) + ') is not a function');
		
		var callback = function(p) {
			var pos = posFn(p);
			if(motion.util._isVector(pos))
				p.position = pos;
			else if(typeof(pos) != 'undefined')
				throw('in motion.Position callback: posFn should return ' +
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
 * @param presence The presence to control
 * @param orientFn A function that should return the new orientation of the
 *		presence at any point in time (just return; to leave orientation
 *		unchanged)
 * @param period (optional =defaultPeriod) The period at which the object's
 *		orientation is updated
 */
motion.Orientation = motion.Motion.extend({
	init: function(presence, orientFn, period) {
		if(typeof(orientFn) !== 'function')
			throw('second argument "orientFn" to motion.Orientation (' +
					system.core.pretty(orientFn) + ') is not a function');
		
		var callback = function(p) {
			var orient = orientFn(p);
			if(motion.util._isQuat(orient))
				p.orientation = orient;
			else if(typeof(orient) != 'undefined')
				throw('in motion.Orientation callback: orientFn should return ' +
						'a quaternion or undefined (instead got ' +
						std.core.pretty(orient) + ')');
		};
		this._super(presence, callback, period);
	}
});

/**
 * @class A controller for manipulating the orientation velocity of a
 * 		presence.
 * @param presence The presence to control
 * @param oVelFn A function that should return the new orientation velocity
 * 		of the presence at any point in time (just return; to leave orientation
 *		unchanged)
 * @param period (optional =defaultPeriod) The period at which the object's
 * 		orientation velocity is updated
 */
motion.OrientationVel = motion.Motion.extend({
	init: function(presence, oVelFn, period) {
		if(typeof(oVelFn) !== 'function')
			throw('second argument "oVelFn" to motion.Orientation (' +
					system.core.pretty(oVelFn) + ') is not a function');
		
		var callback = function(p) {
			var oVel = oVelFn(p);
			if(motion.util._isQuat(oVel))
				p.orientationVel = oVel;
			else if(typeof(oVel) != 'undefined')
				throw('in motion.Orientation callback: oVelFn should return ' +
						'a quaternion or undefined (instead got ' +
						std.core.pretty(oVel) + ')');
		};
		this._super(presence, callback, period);
	}
});

/**
 * @class A controller that applies angular acceleration to a presence.
 * @param presence The presence to control
 * @param oAccelFn A function that should return the angular acceleration on
 *      the presence at any point in time, as a vector or a quaternion.  If
 *      oAccelFn returns undefined ("return;"), the acceleration will be
 *      unchanged from the last call.  If accelFn itself is undefined (or not
 *      provided), the controller will use the value of
 *      presence.orientationAccel, which if assigned externally, should be a
 *      vector (axis with magnitude).
 * @param period (optional =defaultPeriod) The period at which the
 *		angular acceleration is updated
 */
motion.OrientationAccel = motion.Motion.extend({
	init: function(presence, oAccelFn, period) {
		var self = this;
		if(typeof(presence.orientationAccel) === 'undefined')
			presence.orientationAccel = <0, 0, 0>;
		if(typeof(oAccelFn) === 'undefined')
			oAccelFn = function(p) { return p.orientationAccel; };
		else if(typeof(oAccelFn) !== 'function')
			throw('second argument "oAccelFn" to motion.Acceleration (' +
					system.core.pretty(oAccelFn) +
					' is not a function or undefined');
		
		var callback = function(p) {
			var oAccel = oAccelFn(p);
			if(motion.util._isVector(oAccel))
				p.orientationAccel = oAccel;
            else if(motion.util._isQuat(oAccel))
                p.orientationAccel = oAccel.axis().scale(oAccel.length());
			else if(typeof(oAccel) != 'undefined')
				throw('in motion.Acceleration callback: oAccelFn should return ' +
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
 * Accelerates a presence downward under a constant gravitational force.
 *
 * @param presence The presence to accelerate
 * @param accel (optional =<0, -defaultGravity, 0>) The acceleration of
 *		gravity (as either a scalar quantity or a vector).  This may be
 *		changed later through <code>controller.accel</code>, but only as a
 *		vector.
 * @param period (optional =defaultPerid) The period at which the presence's
 *		velocity is updated
 */
motion.Gravity = motion.Acceleration.extend({
	init: function(presence, accel, period) {
		var self = this;
		
		if(typeof(accel) === 'number')
			self.accel = <0, -accel, 0>;
		else
			self.accel = accel || <0, -motion.defaultGravity, 0>;
		
		this._super(presence, function() { return self.accel; }, period);
	}
});

/**
 * Accelerates a presence under a harmonic spring force.
 *
 * @param presence The presence to control
 * @param anchor The anchor point around which the presence oscillates.  This
 *		can be a vector (point in space) or another presence or visible (which
 * 		will be examined as its position changes).  It can be changed later 
 *		through <code>controller.anchor</code>.
 * @param stiffness The stiffness or "spring constant" of the spring force --
 *		the greater the stiffness, the greater the force at the same distance
 * @param damping (optional =0) The damping or "friction" of the spring motion
 * @param eqLength (optional =0) The equilibrium length of the spring; if
 *		positive, the presence will be accelerated *away* from the anchor
 *		point if it gets too close
 * @param period (optional =defaultPeriod) The period at which the presence's
 *		velocity is updated
 */
motion.Spring = motion.Acceleration.extend({
	init: function(presence, anchor, stiffness, damping, eqLength, period) {
		var self = this;
		
		self.stiffness = stiffness;
		self.eqLength = eqLength || 0;
		self.damping = damping || 0;
		
		self.anchor = anchor;
		var anchorFn;
		if(typeof(self.anchor) === 'object' && 'x' in self.anchor)
			anchorFn = function() {	return self.anchor; };
		else if(typeof(anchor) === 'object' && 'position' in self.anchor)
			anchorFn = function() { return self.anchor.position; };
		else
			throw("Second argument 'anchor' to motion.Spring constructor ('" +
					std.core.pretty(anchor) +
					"') is not a vector or presence");
		
		var accelFn = function(p) {
			var mass = motion.util._mass(p);
		
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
 * @param presence The presence whose collisions are to be detected (the
 *		"colliding presence")
 * @param testFn A function that should detect any collisions when called
 *		repeatedly and return one in the form of a "collision object"
 * @param responseFn A function to be called when a collision happens
 * @param period (optional =defaultPeriod) The period at which to check for
 *		collisions
 *
 * @see collision.em
 */
motion.Collision = motion.Motion.extend({
	init: function(presence, testFn, responseFn, period) {
		var self = this;
		
		self.testFn = testFn;
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

        self.collisionHandler = (onCollisionMessage <<
                [{'action':'collision':},
                {'id':presence.toString():},
                {'collision'::}]);
		
		var testCollision = function(p) {
				var collision = self.testFn(p);
            if(collision) {
                {
                    action: 'collision',
                    id: collision.self.id,
                    collision: collision
                } >> system.createVisible(collision.self.id) >> [];

                if(typeof(collision.other.id) === 'string') {
                    {
                        action: 'collision',
                        id: collision.other.id,
                        collision: collision
                    } >> system.createVisible(collision.other.id) >> [];
				}
			}
		};
		
		this._super(presence, testCollision, period);
	},

    suspend: function() {
        self.collisionHandler.suspend();
        this._super();
    },

    reset: function() {
        self.colisionHandler.reset();
        this._super();
	}
});

/**
 * The default vector to use as "up" in making a presence look forward.
 * @constant
 */
motion.defaultUp = <0, 1, 0>;

/**
 * The default orientation of an presence.
 * @constant
 */
motion.defaultOrientation = new util.Quaternion();

/**
 * @class A controller that always points an object in the direction it is
 *		currently moving.
 *
 * @param presence The presence to control.
 * @param baseOrientation The orientation for the presence that makes it point
 *		along the negative z axis.  This can be used to reorient "sideways"
 *		meshes taken from the CDN.
 * @param up (optional =defaultUp) The direction that the presence will use to
 * 		orient itself so it is right-side up in addition to facing forward
 * @param period (optional =defaultPeriod) The period at which to update the
 * 		presence's orientation.
 */
motion.LookForward = motion.Orientation.extend({
	init: function(presence, baseOrientation, up, period) {
		up = up || motion.defaultUp;
		baseOrientation = baseOrientation || motion.defaultOrientation;
		
		// This section was an attempt to guess an orientation velocity using
		// the 'accel' field created by some of the other controllers, but it
		// just seems to make the jittering worse.
		// TODO: make this feature work
		/*
		var useAccel = function(p) {
			if(!('accel' in p))
				return;
			
			var omega = p.velocity.cross(p.accel).
					div(p.velocity.lengthSquared());
			return (new util.Quaternion(omega.normal(), 1)).
					scale(omega.length());
		};
		this.oVelController = new motion.OrientationVel(presence, useAccel,
				period);
		*/
		
		var lookForward = function(p) {
			if(p.velocity.length() < 1e-8)
				return;
			
			return (util.Quaternion.fromLookAt(p.velocity, up)).
					mul(baseOrientation);
		};
		this._super(presence, lookForward, period);
	}
});

/**
 * @class A controller that applies force and optionally torque to a presence.
 * @param presence The presence to control
 * @param force The force to apply, as a vector or as a function that returns a
 *      vector when called.
 * @param position (optional =presence's position) The position (in world
 *      coordinates) at which to apply the force, as a vector, a visible whose
 *      position is tracked, or a function that returns a vector when called.
 *      If position is undefined or not given, the position of the presence is
 *      used (and therefore no torque is exerted).
 * @param period The period at which to change the presence's velocities when
 *      applying force and torque.
 */
motion.ForceTorque = motion.Acceleration.extend({
    init: function(presence, force, position, period) {
        var forceFn;
        if(typeof(force) === 'function')
            forceFn = force;
        else if(motion.util._isVec(force))
            forceFn = function(p) { return force; };
        else
			throw("Second argument 'force' to motion.ForceTorque constructor ('" +
					std.core.pretty(force) +
					"') is not a function or vector");

        var posFn;
        if(typeof(position) === 'function')
            posFn = position;
        else if(typeof(position) === 'undefined')
            posFn = function(p) { return p.position; };
        else if(motion.util._isVec(position))
            posFn = function(p) { return position; };
        else if(motion.util._isVisible(position))
            posFn = function(p) { return position.position; };
        else
			throw("Second argument 'position' to motion.ForceTorque constructor ('" +
					std.core.pretty(position) +
					"') is not a vector, visible, function, or undefined");
        
        var accelFn = function(p) {
            return <0, 0, 0>;
            // return forceFn(p).div(motion.util._mass(p));
        };
        var oAccelFn = function(p) {
            return p.orientation.inverse().mul((posFn(p) -
					p.position).cross(forceFn(p))).div(motion.util._inertia(p));
        };

        this.oAccel = new motion.OrientationAccel(presence, oAccelFn, period);
        this._super(presence, accelFn, period);
    }
});
