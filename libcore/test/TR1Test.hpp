/*  Sirikata Tests -- Sirikata Test Suite
 *  TR1Test.hpp
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

#include "util/Platform.hh"
#include <cxxtest/TestSuite.h>
class TR1Test : public CxxTest::TestSuite
{
public:
    void testUnordered (void )
    {
        using namespace tech;
        typedef unordered_map<std::string, unsigned long> Map;
        typedef unordered_set<std::string> Set;
        typedef unordered_map<unsigned long,std::string> RMap;
        Map colors;
        RMap rcolors;
        Set colornames;
        colornames.insert("black");
        colors["black"] = 0x000000ul;
        colornames.insert("red");
        colors["red"] = 0xff0000ul;
        colornames.insert("white");
        colors.insert(Map::value_type("white",0xfffffful));
        colornames.insert("green");
        colors["green"] = 0x00ff00ul;
        colornames.insert("blue");
        colors["blue"] = 0x0000fful;
        colornames.insert("blue");
        colors["blue"] = 0x0000fful;//again just for kicks
        TS_ASSERT_EQUALS(colors["red"],0xff0000ul);
        TS_ASSERT_EQUALS(colors["black"],0x000000ul);
        TS_ASSERT_EQUALS(colors["white"],0xfffffful);
        TS_ASSERT_EQUALS(colors["green"],0x00ff00ul);
        TS_ASSERT_EQUALS(colors["blue"],0x0000fful);
        for (Map::iterator i = colors.begin();
             i != colors.end();
             ++i) {
            rcolors[i->second]=i->first;
            switch (i->second) {
              case 0xff0000ul:
                TS_ASSERT_EQUALS(i->first,"red");
                break;
              case 0x00ff00ul:
                TS_ASSERT_EQUALS(i->first,"green");
                break;
              case 0x0000fful:
                TS_ASSERT_EQUALS(i->first,"blue");
                break;
              case 0x000000ul:
                TS_ASSERT_EQUALS(i->first,"black");
                break;
              case 0xfffffful:
                TS_ASSERT_EQUALS(i->first,"white");
                break;
              default:
                TS_FAIL("Color not matched");
                break;
            }
        }
        for (Set::iterator i=colornames.begin();i!=colornames.end();++i) {
            TS_ASSERT(*i=="red"||*i=="white"||*i=="black"||*i=="blue"||*i=="green");
        }
        TS_ASSERT_EQUALS((int)colornames.size(),5);
        TS_ASSERT_EQUALS((int)colors.size(),5);
        TS_ASSERT_EQUALS((int)rcolors.size(),5);
        TS_ASSERT_DIFFERS(colors.find("white"),colors.end());
        colors.erase(colors.find("white"));
        TS_ASSERT_EQUALS(colors.find("white"),colors.end());
        TS_ASSERT_EQUALS((int)colors.size(),4);
    }
};
