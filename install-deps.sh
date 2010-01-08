#!/bin/bash

opt_update="false"
opt_components_all="true"
opt_components_sirikata="false"
opt_components_prox="false"

# parameters
until [ -z "$1" ]
do
  if [ "$1" == "update" ]; then
    opt_update="true"
    echo "Performing update"
  elif [ "$1" == "sirikata" ]; then
    opt_components_all="false"
    opt_components_sirikata="true"
  elif [ "$1" == "prox" ]; then
    opt_components_all="false"
    opt_components_prox="true"
  else
    echo "Unknown option: $1"
  fi
  shift
done

# if opt_components_all is still marked, mark all for processing
if [ ${opt_components_all} == "true" ]; then
  opt_components_sirikata="true"
  opt_components_prox="true"

  # when we're installing everything and not updating, perform basic system installation tasks
  if [ ${opt_update} == "false" ]; then
    echo formerly sudo apt-get install libtool automake1.9 autoconf patch unzip g++ cmake freeglut-dev jgraph python-pydot
  fi
fi


# make sure we have the basic directory layout
if [ ! -e dependencies ]; then
    mkdir dependencies
fi
cd dependencies

deps_dir=`pwd`


# sirikata
if [[ "x${ARCH}" == "x" ]]; then
  ARCH=`uname -m`
fi


sirikata_commit="9d85d4acccd165203787d551207a87f4305789c6"


if [ ${opt_components_sirikata} == "true" ]; then

  if [ ${opt_update} != "true" ]; then
    if [ -e sirikata ]; then
      rm -rf sirikata
    fi
    if [ -e installed-sirikata ]; then
      rm -rf installed-sirikata
    fi
    git clone git://github.com/sirikata/sirikata.git sirikata
    cd sirikata
    git fetch origin
    make ARCH=${ARCH} minimaldepends
  else
    cd sirikata
    git checkout master
    git pull origin
    git fetch origin
  fi
  git branch -D cbr_pinned
  git branch cbr_pinned ${sirikata_commit}
  git checkout cbr_pinned
  git submodule init
  git submodule update
  cd build/cmake
  # debug
  rm -f CMakeCache.txt
  mkdir -p debugdir
  cd debugdir
  rm -f CMakeCache.txt
  cmake -DCMAKE_INSTALL_PREFIX=${deps_dir}/installed-sirikata -DCMAKE_BUILD_TYPE=Debug ..
  make -j2
  make install
  cd ..
  # release
  mkdir -p releasedir
  cd releasedir
  rm CMakeCache.txt
  cmake -DCMAKE_INSTALL_PREFIX=${deps_dir}/installed-sirikata -DCMAKE_BUILD_TYPE=Release ..
  make -j2
  make install
  cd ..
  # we need to manually copy over the protobuf libs because sirikata doesn't handle this properly yet
  cp ${deps_dir}/sirikata/dependencies/installed-protobufs/lib/libproto* ${deps_dir}/installed-sirikata/lib/
  cd ../../..

fi # opt_components_sirikata


# prox
if [ ${opt_components_prox} == "true" ]; then

  if [ ${opt_update} != "true" ]; then
    if [ -e prox ]; then
      rm -rf prox
    fi
    if [ -e installed-prox ]; then
      rm -rf installed-prox
    fi
    git clone git@ahoy.stanford.edu:prox.git
    cd prox
  else
    cd prox
    git reset --hard HEAD
    git pull origin master:master
  fi

  cd build

  # Debug
  cmake -DCMAKE_INSTALL_PREFIX=${deps_dir}/installed-prox -DCMAKE_BUILD_TYPE=Debug -DBOOST_ROOT=${deps_dir}/sirikata/dependencies/installed-boost .
  make clean
  make -j2
  make install

  # Release
  cmake -DCMAKE_INSTALL_PREFIX=${deps_dir}/installed-prox -DCMAKE_BUILD_TYPE=Release -DBOOST_ROOT=${deps_dir}/sirikata/dependencies/installed-boost .
  make clean
  make -j2
  make install

fi # opt_components_prox
