#!/bin/bash

# A simple wrapper that will run CMake but also try to get additional
# tools to work with your build (e.g. ccache) since these require
# specifying a different compiler command and CMake seems to not like
# changing the compiler in its own script.

DIR=`pwd`

CCACHE=`which ccache`
GPP=`which g++`
GCC=`which gcc`
DARWIN=`uname | grep Darwin`

# CMake on mac doesn't play well with overriding the compiler so we
# explicitly disable it there.
if [ "$CCACHE" != "" -a "$GPP" != "" -a "$GCC" != "" -a "x$DARWIN" == "x" ]; then
    echo "Using CCache"
    CC="$CCACHE $GCC" CXX="$CCACHE $GPP" cmake $*
    exit 0
fi

# If nothing else works, use the default cmake
echo "Using Standard Build"
cmake $*