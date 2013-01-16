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
        TS_ASSERT_EQUALS(url.query(), "");
        TS_ASSERT_EQUALS(url.fragment(), "");
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
        TS_ASSERT_EQUALS(url.query(), "");
        TS_ASSERT_EQUALS(url.fragment(), "");
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
        TS_ASSERT_EQUALS(url.query(), "");
        TS_ASSERT_EQUALS(url.fragment(), "");
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
        TS_ASSERT_EQUALS(url.query(), "");
        TS_ASSERT_EQUALS(url.fragment(), "");
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
        TS_ASSERT_EQUALS(url.query(), "");
        TS_ASSERT_EQUALS(url.fragment(), "");
    }

    void testParseHttpWithQueryString(void) {
        Sirikata::Transfer::URL url("http://www.google.com/path/to/file?param1=value1");
        TS_ASSERT_EQUALS(url.proto(), "http");
        TS_ASSERT_EQUALS(url.host(), "www.google.com");
        TS_ASSERT_EQUALS(url.hostname(), "www.google.com");
        TS_ASSERT_EQUALS(url.username(), "");
        TS_ASSERT_EQUALS(url.basepath(), "path/to");
        TS_ASSERT_EQUALS(url.filename(), "file");
        TS_ASSERT_EQUALS(url.fullpath(), "/path/to/file");
        TS_ASSERT_EQUALS(url.query(), "param1=value1");
        TS_ASSERT_EQUALS(url.fragment(), "");
    }

    void testParseHttpWithFragmentID(void) {
        Sirikata::Transfer::URL url("http://www.google.com/path/to/file#fragmentID");
        TS_ASSERT_EQUALS(url.proto(), "http");
        TS_ASSERT_EQUALS(url.host(), "www.google.com");
        TS_ASSERT_EQUALS(url.hostname(), "www.google.com");
        TS_ASSERT_EQUALS(url.username(), "");
        TS_ASSERT_EQUALS(url.basepath(), "path/to");
        TS_ASSERT_EQUALS(url.filename(), "file");
        TS_ASSERT_EQUALS(url.fullpath(), "/path/to/file");
        TS_ASSERT_EQUALS(url.query(), "");
        TS_ASSERT_EQUALS(url.fragment(), "fragmentID");
    }

    void testParseHttpWithEverything(void) {
        Sirikata::Transfer::URL url("http://ewencp@www.google.com:9000/path/to/file?param1=value1#fragmentID");
        TS_ASSERT_EQUALS(url.proto(), "http");
        TS_ASSERT_EQUALS(url.host(), "www.google.com:9000");
        TS_ASSERT_EQUALS(url.hostname(), "www.google.com");
        TS_ASSERT_EQUALS(url.username(), "ewencp");
        TS_ASSERT_EQUALS(url.basepath(), "path/to");
        TS_ASSERT_EQUALS(url.filename(), "file");
        TS_ASSERT_EQUALS(url.fullpath(), "/path/to/file");
        TS_ASSERT_EQUALS(url.query(), "param1=value1");
        TS_ASSERT_EQUALS(url.fragment(), "fragmentID");
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
        TS_ASSERT_EQUALS(url.query(), "");
        TS_ASSERT_EQUALS(url.fragment(), "");
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
        TS_ASSERT_EQUALS(url.query(), "");
        TS_ASSERT_EQUALS(url.fragment(), "");
    }

    void testToString(void) {
        // These all assume the parsing tests pass. They just verify
        // the output matches the original (clean) input formatting

        // Simple domain, no path
        {
            Sirikata::Transfer::URL url("http://www.google.com");
            TS_ASSERT_EQUALS(url.toString(), "http://www.google.com/");
        }

        // Simple domain + path
        {
            Sirikata::Transfer::URL url("http://www.google.com/path/to/file");
            TS_ASSERT_EQUALS(url.toString(), "http://www.google.com/path/to/file");
        }

        // Port in host
        {
            Sirikata::Transfer::URL url("http://www.google.com:9000/path/to/file");
            TS_ASSERT_EQUALS(url.toString(), "http://www.google.com:9000/path/to/file");
        }

        // Username
        {
            Sirikata::Transfer::URL url("http://ewencp@www.google.com/path/to/file");
            TS_ASSERT_EQUALS(url.toString(), "http://ewencp@www.google.com/path/to/file");
        }

        // Query string
        {
            Sirikata::Transfer::URL url("http://www.google.com/path/to/file?param1=value1");
            TS_ASSERT_EQUALS(url.toString(), "http://www.google.com/path/to/file?param1=value1");
        }

        // Fragment id
        {
            Sirikata::Transfer::URL url("http://www.google.com/path/to/file#fragmentID");
            TS_ASSERT_EQUALS(url.toString(), "http://www.google.com/path/to/file#fragmentID");
        }

        // Everything
        {
            Sirikata::Transfer::URL url("http://ewencp@www.google.com:9000/path/to/file?param1=value1#fragmentID");
            TS_ASSERT_EQUALS(url.toString(), "http://ewencp@www.google.com:9000/path/to/file?param1=value1#fragmentID");
        }
    }

};
