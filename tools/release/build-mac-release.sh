#!/bin/bash
# Build a release for mac based on the current checkout.
#
# build-mac-release.sh x.y.z
#
# Note that this doesn't take care of dependencies -- it assumes you've already taken care of them and have a working build.

DIR=`pwd`
TMPDIR=/tmp
VERSION=$1
BUILD_TYPE="Release"
BUILD_TYPE_TAG=""

until [ -z "$1" ] # until all parameters used up
do
case "$1" in
      --debug)
          BUILD_TYPE="Debug"
          BUILD_TYPE_TAG="-debug"
          ;;
      *)
          VERSION="$1"
          ;;
  esac
shift
done

NAMEDIR=sirikata_mac
INSTALLDIR=${TMPDIR}/${NAMEDIR}
TAR_NAME=sirikata-${VERSION}-mac${BUILD_TYPE_TAG}.tar
ARCHIVE_NAME=${TAR_NAME}.gz

# Clean out working directory to make sure we don't get any old data we don't want
rm -rf ${INSTALLDIR}

# Make sure we're up to date
git submodule update --init --recursive

# Clear out old build setup and run the build
cd ${DIR}/build/cmake
rm CMakeCache.txt
./cmake_with_tools.sh -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DCMAKE_INSTALL_PREFIX=${INSTALLDIR} .
make -j2 install

# Package it all up
cd ${TMPDIR}
# tar, gz
tar -cvvf ${TAR_NAME} ${NAMEDIR}
gzip --best ${TAR_NAME}
# Copy back to the directory we started from
cd ${DIR}
mv ${TMPDIR}/${ARCHIVE_NAME} .
# Cleanup
rm -rf ${INSTALLDIR}