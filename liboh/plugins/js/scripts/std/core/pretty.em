/*  Sirikata
 *  pretty.js
 *
 *  Copyright (c) 2011, Ewen Cheslack-Postava
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

if (typeof(std) === "undefined") std = {};
if (typeof(std.core) === "undefined") std.core = {};

/** Converts an object to a string in a 'pretty' format, i.e. so it is
 *  human-readable, multiple lines, and handles indentation. If the
 *  object is not a tree (i.e. it has circular references in it), then
 *  the normal toString version will be returned.  Generally this
 *  should only be used on small, tree-like objects.
 */
std.core.pretty = function(obj) {
    var visited = [];

    // Fast path -- if its not an object or array, we can just do the normal conversion
    if (typeof(obj) !== "object" || obj === null)
        return '' + obj;

    var objectFields = function(obj) {
        if (typeof(obj.__prettyPrintFields__) == "function")
            return obj.__prettyPrintFields__();
        else
            return Object.getOwnPropertyNames(obj);
    };
    var longObject = function(obj) {
        return (objectFields(obj).length > 3);
    };

    var output = '';
    var obj_stack = [ {obj: obj, idx: -1} ];
    var indent = '';
    while(obj_stack.length != 0) {
        var cur = obj_stack.pop();
        // Check if we need to start this object
        if (cur.idx == -1) {
            // Start the object
            output += '{';
            indent += ' ';
            obj_stack.push( {obj: cur.obj, idx: cur.idx+1} );
        }
        else if (cur.idx < objectFields(cur.obj).length) {
            // Setup processing of next child

            // Add a comma and either space or newline as appropriate
            if (cur.idx != 0)
                output += ',';
            if (longObject(cur.obj))
                output += '\n' + indent;
            else
                output += ' ';

            obj_stack.push( {obj: cur.obj, idx: cur.idx+1} );
            // And process this one, possibly triggering recursion
            var key = objectFields(cur.obj)[cur.idx];
            var child = cur.obj[key];
            output += key + ': ';
            if (typeof(child) === "object" && child !== null)
                obj_stack.push( {obj: child, idx: -1} );
            else
                output += child;
        }
        else {
            // Close this object
            indent = indent.substr(1);

            if (longObject(cur.obj))
                output += '\n' + indent;
            else
                output += ' ';

            output += '}';

            // No object stack push, we've finished with this object
        }
    }

    return output;
};
