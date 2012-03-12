// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <cxxtest/TestSuite.h>
#include <sirikata/core/util/Paths.hpp>

class PathsTest : public CxxTest::TestSuite
{
public:

    // We obviously can't test that paths are correct since this will vary
    // depending on where you've built / run from. Instead, we just sanity check
    // that we don't get invalid results.

    void testGetInvalidKey(void) {
        Sirikata::String res = Sirikata::Path::Get(Sirikata::Path::PATH_END);
        TS_ASSERT_EQUALS(res, "");
    }

    void testGetExeFile(void) {
        Sirikata::String res = Sirikata::Path::Get(Sirikata::Path::FILE_EXE);
        TS_ASSERT_DIFFERS(res, "");
    }

    void testGetExeDir(void) {
        Sirikata::String res = Sirikata::Path::Get(Sirikata::Path::DIR_EXE);
        TS_ASSERT_DIFFERS(res, "");
    }

    void testGetExeBundleDir(void) {
        Sirikata::String res = Sirikata::Path::Get(Sirikata::Path::DIR_EXE_BUNDLE);
        TS_ASSERT_DIFFERS(res, "");
    }

    void testGetExeDirMatchesFile(void) {
        Sirikata::String exe_file = Sirikata::Path::Get(Sirikata::Path::FILE_EXE);
        Sirikata::String exe_dir = Sirikata::Path::Get(Sirikata::Path::DIR_EXE);
        TS_ASSERT(exe_file.find(exe_dir) != String::npos);
    }

    void testGetDirCurrent(void) {
        Sirikata::String res = Sirikata::Path::Get(Sirikata::Path::DIR_CURRENT);
        TS_ASSERT_DIFFERS(res, "");
        TS_ASSERT_DIFFERS(res, ".");
    }
};
