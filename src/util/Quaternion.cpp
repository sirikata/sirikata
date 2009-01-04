/*     Iridium Utilities -- Math Library
 *  Quaternion.cpp
 *
 *  Copyright (c) 2009, Daniel Reiter Horn
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
 *  * Neither the name of Iridium nor the names of its contributors may
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
#include <string>
#include <sstream>
#include "Quaternion.hpp"
#include <cmath>
namespace Iridium {
Quaternion::Quaternion(const Vector3<Quaternion::scalar>&axis, Quaternion::scalar angle) {
        float sinHalfAngle=sin(angle*.5);
        x=sinHalfAngle*axis.x;
        y=sinHalfAngle*axis.y;
        z=sinHalfAngle*axis.z;
        w=cos(angle*.5);
    }

Vector3<Quaternion::scalar> Quaternion::xAxis(void) const
{                                                                           
    //scalar fTx  = 2.0*x;                                                    
    scalar fTy  = 2.0*y;                                                      
    scalar fTz  = 2.0*z;                                                      
    scalar fTwy = fTy*w;                                                      
    scalar fTwz = fTz*w;                                                      
    scalar fTxy = fTy*x;                                                      
    scalar fTxz = fTz*x;                                                      
    scalar fTyy = fTy*y;                                                       
    scalar fTzz = fTz*z;                                                      
    
    return Vector3<scalar>(1.0-(fTyy+fTzz), fTxy+fTwz, fTxz-fTwy);                  
}         

Vector3<Quaternion::scalar> Quaternion::yAxis(void) const
{                                                                           
    scalar fTx  = 2.0*x;                                                      
    scalar fTy  = 2.0*y;                                                      
    scalar fTz  = 2.0*z;                                                      
    scalar fTwx = fTx*w;                                                      
    scalar fTwz = fTz*w;                                                      
    scalar fTxx = fTx*x;                                                      
    scalar fTxy = fTy*x;                                                      
    scalar fTyz = fTz*y;                                                      
    scalar fTzz = fTz*z;                                                      
    
    return Vector3<scalar>(fTxy-fTwz, 1.0-(fTxx+fTzz), fTyz+fTwx);                  
}
Vector3<Quaternion::scalar> Quaternion::zAxis(void) const {
        scalar fTx  = 2.0*x;
        scalar fTy  = 2.0*y;
        scalar fTz  = 2.0*z;
        scalar fTwx = fTx*w;
        scalar fTwy = fTy*w;
        scalar fTxx = fTx*x;
        scalar fTxz = fTz*x;
        scalar fTyy = fTy*y;
        scalar fTyz = fTz*y;                                                    
        return Vector3<scalar>(fTxz+fTwy, fTyz-fTwx, 1.0-(fTxx+fTyy));
}
//-----------------------------------------------------------------------
void Quaternion::ToAngleAxis (Quaternion::scalar& returnAngleRadians,
                              Vector3<Quaternion::scalar>& returnAxis) const
{
    scalar fSqrLength = x*x+y*y+z*z;
    scalar tmpw=w;
    scalar eps=1e-06;
    if (tmpw>1.0&&tmpw<1.0+eps)
        tmpw=1.0;
    if (tmpw<-1.0&&tmpw>-1.0-eps)
        tmpw=-1.0;
    if ( fSqrLength > 1e-08 && tmpw<=1 && tmpw>=-1)
    {
        returnAngleRadians = 2.0*acos(tmpw);
        scalar length = sqrt(fSqrLength);
        returnAxis.x = x/length;
        returnAxis.y = y/length;
        returnAxis.z = z/length;
    }
    else
    {
        returnAngleRadians = 0.0;
        returnAxis.x = 1.0;
        returnAxis.y = 0.0;
        returnAxis.z = 0.0;
    }
}

}
