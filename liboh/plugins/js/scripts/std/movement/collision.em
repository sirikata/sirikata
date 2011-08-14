/*  Sirikata
 *  collision.em
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

if(typeof(coll) === 'undefined')
    /**
     * @namespace
     * A library of standard collision functions for the generic Collision motion
     * controller<br /><br />
     *
     * The typical usage of the Collision motion controller is<br /><code>
     *      new motion.Collision(presence, test, response);</code></br>
     * <code>test</code> and <code>response</code> are functions called by
     * Collision's machinery.  These two callback functions will frequently
     * fall into a few different general categories that are implemented here
     * to avoid code duplication.<br /><br />
 *
 * The coll module contains "metafunctions" -- themselves functions, but which
 * should not be passed directly into Collision; rather, when called with
 * certain arguments, they return a function that can be passed to Collision.
 * These arguments are used to customize the functions to meet different
     * requirements of the client.  For example:<br /><code>
 *      new motion.Collision(presence, coll.TestSpheres(otherVisibles),
     *              coll.Bounce(.8));</code><br />
 * Be careful to always call the metafunctions, even when using all defaults
     * (when sometimes no parameters are necessary):<br /><code>
 *      new motion.Collision(presence, coll.TestSpheres(otherVisibles),
     *              coll.Bounce()); // not just ...coll.Bounce); !</code><br />
 */
    coll = {};

// TODO: this is a duplicate of motion.util.mass -- combine the two
coll._mass = function(p) {
    if(typeof(p) === 'string')
        p = system.createVisible(p);

    if('mass' in p)
        return p.mass;
    else if('physics' in p && 'mass' in p.physics)
        return p.physics.mass;
    else
        return 1;
}

/* Standard test metafunctions
 * ---------------------------
 * Every test metafunction takes in some extra arguments and returns a
 * function that can be used in a Collision motion controller.  The function
 * that is returned takes one parameter, the presence to test, and returns a
 * collision object.  Collision objects contain these fields:
 *      self - the id of the presence that was being tested for collisions
 *      other - the id of the visible that self recently collided with
 *      normal - a vector pointing outwards from other, perpendicular to the
 *          surface of the collision
 *      impact - the difference in the velocities of the two bodies before
 *          collision (self.velocity - other.velocity)
 *      position - the (approximate) contact point of the two colliding bodies
 */

/**
 * @param {Array&lt;visible&gt;} visibles All other visibles to detect collisions
 *      against
 * @return {function(presence)->object} A collision test function that tests
 *      the colliding presence (passed to the Collision motion controller)
 *      against the visibles listed in the parameter visibles using bounding
 *      spheres obtained from the scale field.  Collision normals, etc. are
 *      reported as if the presence and all visibles are spherical.
 */
coll.TestSpheres = function(visibles) {
    return function(presence) {
        for(v in visibles) {
            if(visibles[v].toString() === presence.toString())
                continue;
            
            var approach = presence.scale + visibles[v].scale;
            var disp = presence.position - visibles[v].position;
            if(disp.lengthSquared() < approach * approach &&
                    disp.dot(presence.velocity) <= 0) {
                var impact = approach - disp.length();
                var collision = {
                    self: {
                        id: presence.toString(),
                        velocity: presence.velocity,
                        mass: coll._mass(presence)
                    },
                    other: {
                        id: visibles[v].toString(),
                        velocity: visibles[v].velocity,
                        mass: coll._mass(visibles[v])
                    },
                    normal: disp.normal(),
                    position: presence.position + disp.scale(presence.scale - impact / 2)
                };
                return collision;
            }
        }
    };
};

/**
 * @param {Array&lt;object&gt;} planes An array of planes to detect collisions
 *      against.  Each plane is specified as an object with two
 *      {@link util.Vec3} fields: <code>anchor</code>, a point on the plane,
 *      and <code>normal</code>, a unit vector perpendicular to the plane.
 * @return {function(presence)->object} A collision test function that tests
 *      the colliding presence (passed to the Collision motion controller)
 *      against the visibles listed in <code>visibles</code> using bounding
 *      spheres obtained from the <code>scale</code> field.  Collision normals,
 *      etc. are reported as if the presence is spherical.
 */
coll.TestSphereToPlanes = function(planes) {
    return function(presence) {
        for(p in planes) {
            var disp = presence.position - planes[p].anchor;
            var distance = disp.dot(planes[p].normal);
            if(distance < presence.scale &&
                    planes[p].normal.dot(presence.velocity) <= 0) {
                var collision = {
                    self: {
                        id: presence.toString(),
                        velocity: presence.velocity,
                        mass: coll._mass(presence)
                    },
                    other: {
                        plane: planes[p],
                        velocity: <0, 0, 0>,
                        mass: 0
                    },
                    normal: planes[p].normal,
                    position: presence.position - planes[p].normal.scale(distance)
                };
                return collision;
            }
        }
    };
};

/**
 * @param {util.Vec3} upper The position of the corner of the axis-aligned
 *      bounding box on the most positive side in all three axes
 * @param {util.Vec3} lower The position of the corner on the most negative
 *      side in all three axes
 * @return {function(presence)->object} A collision test function that tests
 *      the colliding presence (passed to the Collision motion controller)
 *      against the sides of an axis-aligned bounding box defined by the two
 *      corners specified.  Collision normals, etc. are reported as if the
 *      presence is spherical.
 */
coll.TestBounds = function(upper, lower) {
    return coll.TestSphereToPlanes([
        {anchor: upper, normal: <-1, 0, 0>},
        {anchor: upper, normal: <0, -1, 0>},
        {anchor: upper, normal: <0, 0, -1>},
        {anchor: lower, normal: <1, 0, 0>},
        {anchor: lower, normal: <0, 1, 0>},
        {anchor: lower, normal: <0, 0, 1>}
    ]);
};

/* Standard callback metafunctions
 * -------------------------------
 * Callback metafunctions return functions that can be used as callbacks for
 * the Collision motion controller.  The metafunctions take arguments used
 * to customize the callback function that is returned, which in turn takes a
 * collision object (as defined above in "test metafunctions").
 */

/**
 * @return {function(presence, object)->undefined} A collision response 
 *      function that stops the colliding presence at the point of collision.
 */
coll.Stop = function() {
    return function(presence, collision) {
        presence.velocity = <0, 0, 0>;
        presence.position = presence.position +
                collision.normal.scale(collision.self.velocity -
                collision.other.velocity);
    };
};

/**
 * @param {number} elast The elasticity of collisions.  For normal results,
 *      this should be a number between 0 and 1, inclusive.
 * @return {function(presence, object)->undefined} A collision response
 *      function that bounces the colliding object off the other object at an
 *      equal angle, multiplying its velocity by the given elasticity.
 */
coll.Bounce = function(elast) {
    if(typeof(elast) === 'undefined')
        elast = 1;
    
    return function(presence, collision) {
        if(collision.normal.dot(presence.velocity -
                collision.other.velocity) >= 0 || collision.self.mass == 0)
            return;
        
        if(collision.other.mass == 0)
            var massFactor = 1;
        else
            var massFactor = collision.other.mass / (collision.self.mass +
                    collision.other.mass);
        
        var dv = collision.normal.scale((1 + elast) * massFactor *
                collision.normal.dot(presence.velocity -
                collision.other.velocity))

        presence.velocity = presence.velocity - dv;
    };
};
