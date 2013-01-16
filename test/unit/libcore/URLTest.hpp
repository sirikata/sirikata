// Copyright (c) 2013 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <cxxtest/TestSuite.h>
#include <sirikata/core/transfer/URL.hpp>

class URLTest : public CxxTest::TestSuite
{
public:
    void testParseHttp(void) {
        Sirikata::Transfer::URL url("http://www.google.com");
        TS_ASSERT_EQUALS(url.proto(), "http");
        TS_ASSERT_EQUALS(url.host(), "www.google.com");
        TS_ASSERT_EQUALS(url.hostname(), "www.google.com");
        TS_ASSERT_EQUALS(url.username(), "");
        TS_ASSERT_EQUALS(url.basepath(), "");
        TS_ASSERT_EQUALS(url.filename(), "");
        TS_ASSERT_EQUALS(url.fullpath(), "/");
    }

    void testParseHttpTrailingSlash(void) {
        Sirikata::Transfer::URL url("http://www.google.com/");
        TS_ASSERT_EQUALS(url.proto(), "http");
        TS_ASSERT_EQUALS(url.host(), "www.google.com");
        TS_ASSERT_EQUALS(url.hostname(), "www.google.com");
        TS_ASSERT_EQUALS(url.username(), "");
        TS_ASSERT_EQUALS(url.basepath(), "");
        TS_ASSERT_EQUALS(url.filename(), "");
        TS_ASSERT_EQUALS(url.fullpath(), "/");
    }

    void testParseHttpWithPath(void) {
        Sirikata::Transfer::URL url("http://www.google.com/path/to/file");
        TS_ASSERT_EQUALS(url.proto(), "http");
        TS_ASSERT_EQUALS(url.host(), "www.google.com");
        TS_ASSERT_EQUALS(url.hostname(), "www.google.com");
        TS_ASSERT_EQUALS(url.username(), "");
        TS_ASSERT_EQUALS(url.basepath(), "path/to");
        TS_ASSERT_EQUALS(url.filename(), "file");
        TS_ASSERT_EQUALS(url.fullpath(), "/path/to/file");
    }

    void testParseHttpWithPort(void) {
        Sirikata::Transfer::URL url("http://www.google.com:9000/");
        TS_ASSERT_EQUALS(url.proto(), "http");
        TS_ASSERT_EQUALS(url.host(), "www.google.com:9000");
        TS_ASSERT_EQUALS(url.hostname(), "www.google.com");
        TS_ASSERT_EQUALS(url.username(), "");
        TS_ASSERT_EQUALS(url.basepath(), "");
        TS_ASSERT_EQUALS(url.filename(), "");
        TS_ASSERT_EQUALS(url.fullpath(), "/");
    }

    void testParseHttpWithUser(void) {
        Sirikata::Transfer::URL url("http://ewencp@www.google.com:9000/");
        TS_ASSERT_EQUALS(url.proto(), "http");
        TS_ASSERT_EQUALS(url.host(), "www.google.com:9000");
        TS_ASSERT_EQUALS(url.hostname(), "www.google.com");
        TS_ASSERT_EQUALS(url.username(), "ewencp");
        TS_ASSERT_EQUALS(url.basepath(), "");
        TS_ASSERT_EQUALS(url.filename(), "");
        TS_ASSERT_EQUALS(url.fullpath(), "/");
    }

    void testParseHttpWithEverything(void) {
        Sirikata::Transfer::URL url("http://ewencp@www.google.com:9000/path/to/file");
        TS_ASSERT_EQUALS(url.proto(), "http");
        TS_ASSERT_EQUALS(url.host(), "www.google.com:9000");
        TS_ASSERT_EQUALS(url.hostname(), "www.google.com");
        TS_ASSERT_EQUALS(url.username(), "ewencp");
        TS_ASSERT_EQUALS(url.basepath(), "path/to");
        TS_ASSERT_EQUALS(url.filename(), "file");
        TS_ASSERT_EQUALS(url.fullpath(), "/path/to/file");
    }

    void testParseLocalFile(void) {
        Sirikata::Transfer::URL url("file:///home/user/file");
        TS_ASSERT_EQUALS(url.proto(), "file");
        TS_ASSERT_EQUALS(url.host(), "");
        TS_ASSERT_EQUALS(url.hostname(), "");
        TS_ASSERT_EQUALS(url.username(), "");
        TS_ASSERT_EQUALS(url.basepath(), "home/user");
        TS_ASSERT_EQUALS(url.filename(), "file");
        TS_ASSERT_EQUALS(url.fullpath(), "/home/user/file");
    }

    void testEmpty(void) {
        Sirikata::Transfer::URL url("");
        TS_ASSERT(url.empty());
        TS_ASSERT_EQUALS(url.proto(), "");
        TS_ASSERT_EQUALS(url.host(), "");
        TS_ASSERT_EQUALS(url.hostname(), "");
        TS_ASSERT_EQUALS(url.username(), "");
        TS_ASSERT_EQUALS(url.basepath(), "");
        TS_ASSERT_EQUALS(url.filename(), "");
        TS_ASSERT_EQUALS(url.fullpath(), "/");
    }

    void testToString(void) {
        Sirikata::Transfer::URL url("http://ewencp@www.google.com:9000/path/to/file");
        TS_ASSERT_EQUALS(url.toString(), "http://ewencp@www.google.com:9000/path/to/file");
    }

};
