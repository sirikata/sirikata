#!/bin/bash

DIR=`pwd`
TMPDIR=/tmp
VERSION=$1
NAMEDIR=sirikata-${VERSION}
TREEISH=v${VERSION}
ARCHIVENAME=sirikata-${VERSION}-src.zip

rm -rf ${TMPDIR}/${NAMEDIR}
# Clone, checkout desired revision, and prepare the directory
git clone git://github.com/sirikata/sirikata.git ${TMPDIR}/${NAMEDIR}
cd ${TMPDIR}/${NAMEDIR}
git checkout ${TREEISH}
git submodule update --init --recursive
# Nuke all .git data
find . -name .git | xargs rm -rf
# And package
cd ${TMPDIR}
zip -r --symlinks ${ARCHIVENAME} ${NAMEDIR}
# Copy back to the directory we're starting from
cd $DIR
mv ${TMPDIR}/${ARCHIVENAME} .
# Cleanup
rm -rf ${TMPDIR}/${NAMEDIR}
