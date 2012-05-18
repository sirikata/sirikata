/*  Sirikata
 *  ExpIntegral.hpp
 *
 *  Copyright (c) 2010, Daniel Reiter Horn
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

#ifndef EXPINTEGRAL_HPP_
#define EXPINTEGRAL_HPP_


// Version 1.46 of Boost (at least) has a bad version of Boost.Math
// that makes it unusable (Boost issue #5934). We need to detect this
// and adjust for it. We can't fix the compilation issue and maintain
// functionality, so we skip registering with the factory to cause an
// error and leave a warning if the plugin is loaded.
#include <boost/version.hpp>

#define SIRIKATA_BOOST_MAJOR_VERSION BOOST_VERSION / 100000
#define SIRIKATA_BOOST_MINOR_VERSION BOOST_VERSION / 100 % 1000

#if SIRIKATA_BOOST_MAJOR_VERSION == 1 && SIRIKATA_BOOST_MINOR_VERSION == 46
#define SIRIKATA_BAD_BOOST_ERF 1
#endif


namespace Sirikata {
double integralExpFunction(double k, const Vector3d& xymin, const Vector3d& xymax, const Vector3d& uvmin, const Vector3d& uvmax);
}
#endif
