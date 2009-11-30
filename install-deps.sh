#!/bin/bash

opt_update="false"
opt_components_all="true"
opt_components_sst="false"
opt_components_sirikata="false"
opt_components_prox="false"

# parameters
until [ -z "$1" ]
do
  if [ "$1" == "update" ]; then
    opt_update="true"
    echo "Performing update"
  elif [ "$1" == "sst" ]; then
    opt_components_all="false"
    opt_components_sst="true"
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
  opt_components_sst="true"
  opt_components_sirikata="true"
  opt_components_prox="true"

  # when we're installing everything and not updating, perform basic system installation tasks
  if [ ${opt_update} == "false" ]; then
    echo formerly sudo apt-get install libtool automake1.9 autoconf patch unzip g++ cmake qt4-dev-tools qt4-qmake qt4-qtconfig freeglut-dev jgraph
  fi
fi


# make sure we have the basic directory layout
if [ ! -e dependencies ]; then
    mkdir dependencies
fi
cd dependencies

deps_dir=`pwd`


# sst
if [ ${opt_components_sst} == "true" ]; then

  if [ ${opt_update} != "true" ]; then
    if [ -e sst ]; then
      rm -rf sst
    fi
    if [ -e installed-sst ]; then
      rm -rf installed-sst
    fi
    #svn co svn://svn.pdos.csail.mit.edu/uia/trunk/uia/sst
    git clone git@ahoy.stanford.edu:sst.git
    cd sst
    git branch stanford origin/stanford
    git checkout stanford
  else
    cd sst
    git reset --hard HEAD
    git pull origin stanford:stanford
  fi

  misc/setup
  ./configure --prefix=${deps_dir}/installed-sst
  make
  make install
  cd ..

fi # opt_components_sst


# sirikata
if [[ "x${ARCH}" == "x" ]]; then
  ARCH=`uname -m`
fi

sirikata_commit="49b8350bbac5c0d9900bf4e88f52e163b7bd5acd"

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
  rm CMakeCache.txt
  cmake -DCMAKE_INSTALL_PREFIX=${deps_dir}/installed-sirikata -DCMAKE_BUILD_TYPE=Debug .
  make clean
  make -j2
  make install
  # release
  rm CMakeCache.txt
  cmake -DCMAKE_INSTALL_PREFIX=${deps_dir}/installed-sirikata -DCMAKE_BUILD_TYPE=Release .
  make clean
  make -j2
  make install
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
