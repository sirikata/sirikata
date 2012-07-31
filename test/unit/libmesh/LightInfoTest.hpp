/*  Sirikata Tests -- Sirikata Test Suite
 *  LightInfoTest.hpp
 *
 *  Copyright (c) 2008, Daniel Reiter Horn
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
#include <cxxtest/TestSuite.h>
#include <sirikata/mesh/Meshdata.hpp>

using namespace Sirikata;
using namespace Mesh;
class LightInfoTest : public CxxTest::TestSuite
{
	typedef Meshdata::LightInstanceIterator LightInstanceIterator;
public:
    void setUp( void )
    {
    }
    void tearDown( void )
    {
    }
	void vAssert(Vector3f v1, Vector3f v2) {
		TS_ASSERT_EQUALS(v1.x, v2.x);
		TS_ASSERT_EQUALS(v1.y, v2.y);
		TS_ASSERT_EQUALS(v1.z, v2.z);
	}
    void testLightInfo( void ) {
		//test LightInfo functionality
		Meshdata md;
		
		Node ned;
		ned.parent = NullNodeIndex;
		md.nodes.push_back(ned);
		md.rootNodes.push_back(0);

		LightInstance li;
		li.lightIndex = 0;
		li.parentNode = 0;
		
		//LightInfo
		LightInfo info;

		info.setCastsShadow(true);
		info.setLightAmbientColor(Vector3f(100, 234, 234));
		info.setLightDiffuseColor(Vector3f(5, 78, 186));

		info.setLightFalloff(32, 65, 23);
		info.setLightPower(4);
		info.setLightRange(100);

		info.setLightShadowColor(Vector3f(67, 45, 23));
		info.setLightSpecularColor(Vector3f(32, 54, 76));

		info.setLightSpotlightCone(2, 4, 3);
		info.setLightType(LightInfo::SPOTLIGHT);

		md.lightInstances.push_back(li);
		md.lights.push_back(info);

		//LightInstanceIterator
		LightInstanceIterator lithium = md.getLightInstanceIterator();
		uint32 u;
		Matrix4x4f m;
		
		//successes
		bool success = lithium.next(&u, &m);
        TS_ASSERT_EQUALS(success, true);
		TS_ASSERT_EQUALS(u, 0);

		TS_ASSERT_EQUALS(md.lights[0].mCastsShadow, true);
		vAssert(md.lights[0].mAmbientColor, Vector3f(100, 234, 234));
		vAssert(md.lights[0].mDiffuseColor, Vector3f(5, 78, 186));

		TS_ASSERT_EQUALS(md.lights[0].mConstantFalloff, 32);
		TS_ASSERT_EQUALS(md.lights[0].mLinearFalloff, 65);
		TS_ASSERT_EQUALS(md.lights[0].mQuadraticFalloff, 23);
		TS_ASSERT_EQUALS(md.lights[0].mPower, 4);
		TS_ASSERT_EQUALS(md.lights[0].mLightRange, 100);

		vAssert(md.lights[0].mShadowColor, Vector3f(67, 45, 23));
		vAssert(md.lights[0].mSpecularColor, Vector3f(32, 54, 76));

		TS_ASSERT_EQUALS(md.lights[0].mConeInnerRadians, 2);
		TS_ASSERT_EQUALS(md.lights[0].mConeOuterRadians, 4);
		TS_ASSERT_EQUALS(md.lights[0].mConeFalloff, 3);
		TS_ASSERT_EQUALS(md.lights[0].mType, LightInfo::SPOTLIGHT);
    }
};
