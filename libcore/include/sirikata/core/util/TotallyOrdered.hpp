/*  Meru
 *  TotallyOrdered.hpp
 *
 *  Copyright (c) 2009, Stanford University
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
#ifndef __SIRIKATA_TOTALLY_ORDERED_HPP_
#define __SIRIKATA_TOTALLY_ORDERED_HPP_


namespace Sirikata {


/** A base class for totally-ordered classes of objects. All comparison operators are auto-generated from the definitions of
 *  the less-than and equality operators. The generated operators are member functions of the base class. The <em>only</em> way
 *  this class should be used is:
 *
 *  \code
 *  class MyClass : public TotallyOrdered<MyClass>
 *  { ...
 *  \endcode
 */
template <typename T>
class TotallyOrdered
{

  public:

    // From less-than operator
    bool operator> (T const & rhs) const { return (rhs < *static_cast<T const *>(this));   }
    bool operator<=(T const & rhs) const { return !(rhs < *static_cast<T const *>(this));  }
    bool operator>=(T const & rhs) const { return !(*static_cast<T const *>(this) < rhs);  }

    // From equality operator
    bool operator!=(T const & rhs) const { return !(*static_cast<T const *>(this) == rhs); }

};  // class TotallyOrdered


}  // namespace Sirikata


#endif
