/*  Sirikata Tests -- Sirikata Test Suite
 *  OptionTest.hpp
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

#include <cxxtest/TestSuite.h>
#include "options/Options.hpp"
class OptionTest : public CxxTest::TestSuite
{
    typedef Sirikata::OptionSet OptionSet;
    typedef Sirikata::OptionValue OptionValue;
    typedef Sirikata::OptionValueType<int> OptionValueTypeInt;
    typedef Sirikata::InitializeOptions InitializeOptions;
public:
    int countString (const char ** test) {
        int count=0;
        while (*test)
            ++test,++count;
        return count;
    }
    void testIntegerOptions( void )
    {
        const char *testString[256][256]={{"test.exe","--one=1","--three=5","--four=4",NULL},
                                    {"test.exe","--one=1","--three=3","--four=4",NULL},
                                    {"test.exe","--one=0","--three=5","--four=4",NULL},
                                    {"test.exe","--one=2","--three=5",NULL},
                                    {"test.exe","--one=-1",NULL},
                                    {"test.exe",NULL}};
        const int answers[256][256]={{1,5,4},{1,3,4},{0,5,4},{2,5,4},{-1,3,4},{1,3,4}};
        OptionValue *one;
        OptionValue *three=OptionSet::referenceOption("testInteger","three");
        InitializeOptions::module("testInteger")
            .addOption(new OptionValue("four","4",OptionValueTypeInt(),"four is the lonliest number"))
            .addOption(one=new OptionValue("one","1",OptionValueTypeInt(),"one is the lonliest number"))
            .addOption(new OptionValue("three","3",OptionValueTypeInt(),"three is the number to which I shalt count"))
            ;
        OptionValue *four=OptionSet::referenceOption("testInteger","four");
        for (int i=0;i<6;++i) {
            OptionSet::getOptions("testInteger")->parse(countString(testString[i]),testString[i]);
            TS_ASSERT_EQUALS(one->as<int>(),answers[i][0]);
            TS_ASSERT_EQUALS(three->as<int>(),answers[i][1]);
            TS_ASSERT_EQUALS(four->as<int>(),answers[i][2]);
        }
    }
};
