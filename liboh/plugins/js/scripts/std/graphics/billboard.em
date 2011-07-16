// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

if (typeof(std) === 'undefined') std = {};
if (typeof(std.graphics) === 'undefined') std.graphics = {};

(
function() {

    /** Billboards are simple, textured quads. They can be textured
     *  with any image URL, can always face the camera or stay fixed
     *  in place, and their shape can be controlled. This class helps
     *  you setup/parse billboard information and the mesh() method
     *  generates a mesh setting for a presence which will use this
     *  billboard.
     *
     *  A very simple billboard might be used like this:
     *
     *    var bb = new Billboard('http://example.com/image.jpg');
     *    system.self.mesh = bb.mesh();
     *
     *  Settings currently supported:
     *   * url - string URL of the image to display
     *   * facing - 'fixed' for a normal billboard that has a fixed
     *     orientation or 'camera' to always face the camera
     *   * aspectRatio - aspect ratio of billboard (width / height),
     *     or -1 to default to the image's ratio
     */
    std.graphics.Billboard = function(url) {
        this.url = url;
        this.facing = 'camera';
        this.aspectRatio = -1;
    };

    /** Get a string description of this billboard which can be
     *  assigned as the mesh for an object.
     */
    std.graphics.Billboard.prototype.mesh = function() {
        var bb = JSON.stringify({
                                    'url' : this.url,
                                    'facing' : this.facing,
                                    'aspect' : this.aspectRatio
                                });
        return 'data:application/json;base64,' + util.Base64.encodeURL(bb);
    };

})();