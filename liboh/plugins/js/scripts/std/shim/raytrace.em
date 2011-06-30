// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

(function() {

     std.raytrace = {};

     // This class monkey patches classes that have raytracing
     // functionality to provide nicer versions of those methods,
     // including variations for different coordinate systems. We do
     // this in a separate file because much of this is duplicated:
     // visibles and presences have the same basic raytracing
     // functionality but don't share the same underlying Emerson
     // code, despite shared C++ code.

     // Convert positions and vectors from a frame into world coords
     var posToWorld = function(pos, from_frame) {
         return from_frame.orientation.mul(pos.mul(from_frame.scale)) + from_frame.position;
     };
     var vecToWorld = function(vec, from_frame) {
         return from_frame.orientation.mul(vec.mul(from_frame.scale));
     };

     // Convert positions and vectors from world coords into a frame
     var posToFrame = function(pos, to_frame) {
         return to_frame.orientation.neg().mul(pos - to_frame.position).mul(1.0/to_frame.scale);
     };
     var vecToFrame = function(vec, to_frame) {
         return to_frame.orientation.neg().mul(vec).mul(1.0/to_frame.scale);
     };

     /** This forms the basis of all our raytracing methods, taking
      *  any object for specifying a coordinate frame, a ray in that
      *  coordinate frame, and a mesh to test against.  It returns a
      *  hit point or undefined
      *
      *  @param {Presence|Visible|null} from_frame object whose
      *  coordinate frame we should raytrace from, i.e. what
      *  coordinate system the ray is in. null indicates world coordinates.
      *  @param {util.Vec3} ray_start starting position for the ray
      *  @param {util.Vec3} ray_dir direction for the ray
      *  @param {Presence|Visible} mesh object we should test the ray
      *  against
      *  @param {Presence|Visible|null} result_frame object whose
      *  coordinate frame we should return the result in. See
      *  from_frame for details.
      *  @returns {util.Vec3} the position of the hit, in the
      *  specified result frame, or undefined if there was no hit
      */
     std.raytrace.raytrace = function(from_frame, ray_start, ray_dir, mesh, result_frame) {
         // Skip unnecessary transformations if the source and target frames are the same
         if (from_frame != mesh) {
             // Transform ray from from_frame to world coords
             if (from_frame) {
                 ray_start = posToWorld(ray_start, from_frame);
                 ray_dir = vecToWorld(ray_dir, from_frame);
             }

             // Transform ray from world coords to mesh coords
             ray_start = posToFrame(ray_start, mesh);
             ray_dir = vecToFrame(ray_dir, mesh);
         }

         // Everything is in object coords now, so we can run the test
         var hit_point = mesh.__raytrace(ray_start, ray_dir);
         // Fast path: if there is no result or if the result frame is
         // the mesh, then we don't need to transform
         if (!hit_point || mesh == result_frame) return hit_point;

         // Transform from mesh frame to world frame
         hit_point = posToWorld(hit_point, mesh);

         // Transform into result frame
         if (hit_point && result_frame)
             hit_point = posToWorld(hit_point, result_frame);

         return hit_point;
     };
     var raytrace = std.raytrace.raytrace;

     // Raytrace *against* this object, using the ray specified in
     // world coordinates and returning in world coordinates. In this
     // case, ray_start is required.
     var raytraceWorld = function(ray_start, ray_dir) {
         return raytrace(null, ray_start, ray_dir, this, null)
     };

     // Raytrace a ray *from* this's perspective *to* the parameter,
     // testing against the
     // parameter. Results are returned in world space.
     // Ray start is optional and defaults to <0, 0, 0>
     var raytraceTo = function(mesh, ray_dir, ray_start) {
         if (!ray_start) ray_start = <0, 0, 0>;
         return raytrace(this, ray_start, ray_dir, mesh, null);
     };

     // Raytrace a ray *against* this, i.e. testing this's mesh for
     // intersections, *from* the perspective of the source
     // specified. You can specify a source coordinate frame, or,
     // more commonly, leave it empty to indicate you want world
     // coordinates. Results are always in the source's frame,
     // i.e. usually you'll just use two parameters and get world
     // coords back.
     // Ray start is optional and defaults to <0, 0, 0>
     var raytraceFrom = function(source, ray_dir, ray_start) {
         if (!ray_start) ray_start = <0, 0, 0>;
         return raytrace(source, ray_start, ray_dir, this, null);
     };

     // Fill in methods on visibles and presences
     var cons = [system.__visible_constructor__, system.__presence_constructor__];
     for(var i in cons) {
         var con = cons[i];

         con.prototype.raytrace = raytraceWorld;
         con.prototype.raytraceFrom = raytraceFrom;
         con.prototype.raytraceTo = raytraceTo;
     }

})();
