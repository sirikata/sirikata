/*  Sirikata
 *  OptionValueListTest.hpp
 *
 *  Copyright (c) 2010, Ewen Cheslack-Postava
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
#include <sirikata/core/options/OptionValue.hpp>

/** Tests parsing of OptionValueLists, particularly focusing on lists of
 *  strings.
 */
class OptionValueListTest : public CxxTest::TestSuite
{
    typedef std::vector<std::string> StringVector;
    typedef Sirikata::OptionValueType<StringVector> OptionValueStringVector;

    StringVector parse(const std::string& val) {
        Any any_result = OptionValueStringVector::lexical_cast(val);
        return any_result.as<StringVector>();
    }

    void parseCheckEmpty(const std::string& val) {
        StringVector result = parse(val);
        TS_ASSERT(result.empty());
    }
    void parseCheckResults1(const std::string& val, const std::string expected[1]) {
        StringVector result = parse(val);
        TS_ASSERT_EQUALS(result.size(), 1);
        TS_ASSERT_EQUALS(result[0], expected[0]);
    }
    void parseCheckResults2(const std::string& val, const std::string expected[2]) {
        StringVector result = parse(val);
        TS_ASSERT_EQUALS(result.size(), 2);
        TS_ASSERT_EQUALS(result[0], expected[0]);
        TS_ASSERT_EQUALS(result[1], expected[1]);
    }
public:
    void testOptionValueListParseEmpty1() {
        parseCheckEmpty("[]");
    }
    void testOptionValueListParseEmpty2() {
        parseCheckEmpty(" [] ");
    }
    void testOptionValueListParseEmpty3() {
        parseCheckEmpty("\t[]\t");
    }

    void testOptionValueListParseSingle1() {
        std::string val = "[a]";
        std::string expected[1] = { "a" };
        parseCheckResults1(val, expected);
    }
    void testOptionValueListParseSingle2() {
        std::string val = "[abcde]";
        std::string expected[1] = { "abcde" };
        parseCheckResults1(val, expected);
    }
    void testOptionValueListParseSingle3() {
        std::string val = "\t[abcde]\t";
        std::string expected[1] = { "abcde" };
        parseCheckResults1(val, expected);
    }
    void testOptionValueListParseSingle4() {
        std::string val = "  [abcde]  ";
        std::string expected[1] = { "abcde" };
        parseCheckResults1(val, expected);
    }

    void testOptionValueListParseSingleNoBrackets1() {
        std::string val = "a";
        std::string expected[1] = { "a" };
        parseCheckResults1(val, expected);
    }
    void testOptionValueListParseSingleNoBrackets2() {
        std::string val = "abcde";
        std::string expected[1] = { "abcde" };
        parseCheckResults1(val, expected);
    }

    void testOptionValueListParseDouble1() {
        std::string val = "[a,b]";
        std::string expected[2] = { "a", "b" };
        parseCheckResults2(val, expected);
    }
    void testOptionValueListParseDouble2() {
        std::string val = "[abcd,efghi]";
        std::string expected[2] = { "abcd", "efghi" };
        parseCheckResults2(val, expected);
    }
    void testOptionValueListParseDouble3() {
        std::string val = "\t[abcd,efghi]\t";
        std::string expected[2] = { "abcd", "efghi" };
        parseCheckResults2(val, expected);
    }
    void testOptionValueListParseDouble4() {
        std::string val = " [abcd,efghi] ";
        std::string expected[2] = { "abcd", "efghi" };
        parseCheckResults2(val, expected);
    }

    void testOptionValueListParseDoubleNoBrackets1() {
        std::string val = "a,b";
        std::string expected[2] = { "a", "b" };
        parseCheckResults2(val, expected);
    }
    void testOptionValueListParseDoubleNoBrackets2() {
        std::string val = "abcde,fghij";
        std::string expected[2] = { "abcde", "fghij" };
        parseCheckResults2(val, expected);
    }

};
