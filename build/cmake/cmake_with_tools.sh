#!/bin/bash

# A simple wrapper that will run CMake but also try to get additional
# tools to work with your build (e.g. ccache) since these require
# specifying a different compiler command and CMake seems to not like
# changing the compiler in its own script.

DIR=`pwd`

CCACHE=`which ccache`
GPP=`which g++`
GCC=`which gcc`

if [ "$CCACHE" != "" -a "$GPP" != "" -a "$GCC" != "" ]; then
    echo "Using CCache"
    CC="$CCACHE $GCC" CXX="$CCACHE $GPP" cmake $*
    exit 0
fi

# If nothing else works, use the default cmake
echo "Using Standard Build"
cmake $*