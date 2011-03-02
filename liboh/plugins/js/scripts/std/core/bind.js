/*  Sirikata
 *  bind.js
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

/** Returns a function that binds the passed function to an object.
 *  Useful for all cases where you need to pass a function argument, but
 *  you expect 'this' to be correctly set.
 *
 *  @param {function(this:Object,...[*])} func  A function object to bind.
 *  @param {Object} object  Instance of some class to become 'this'.
 *  @return {function(...[*])}  A new function that wraps func.apply()
 */
std.core.bind = function(func, object) {
    if (arguments.length==2) {
        delete arguments;
        return function() {
            return func.apply(object, arguments);
        };
    } else {
        var args = new Array(arguments.length-2);
        for (var i = 2; i < arguments.length; ++i) {
            args[i-2]=arguments[i];
        }
        delete arguments;
        return function () {
            var argLen = arguments.length;
            var newarglist = new Array(argLen);
            for (var i = 0; i < argLen; ++i) {
                newarglist[i]=arguments[i];
            }
            return func.apply(object,args.concat(newarglist));
        };
    }
};
