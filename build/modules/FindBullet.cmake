#Sirikata
#FindBullet.cmake
#
#Copyright (c) 2008, Daniel Reiter Horn
#Original: Siddhartha Chaudhuri, 2008
#All rights reserved.
#
#Redistribution and use in source and binary forms, with or without
#modification, are permitted provided that the following conditions are met:
#
#    * Redistributions of source code must retain the above copyright notice,
#      this list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above copyright notice,
#      this list of conditions and the following disclaimer in the documentation
#      and/or other materials provided with the distribution.
#    * Neither the name of the Sirikata nor the names of its contributors
#      may be used to endorse or promote products derived from this software
#      without specific prior written permission.
#
#THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
#ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
#WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
#DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
#ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
#(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
#LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
#ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
#SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


# Searches for an bullet installation
#
# Defines:
#
#   bullet_FOUND          True if bullet was found, else false
#   bullet_LIBRARIES      Libraries to link (without full path)
#   bullet_LIBRARY_DIRS   Directories containing the libraries (-L option)
#   bullet_LDFLAGS        All required linker flags
#   bullet_INCLUDE_DIRS   The directories containing header files (-I option)
#   bullet_CFLAGS         All required compiler flags
#
# If you set bullet_MINIMUM_VERSION, only a version equal to or higher than this will be searched for. (This feature does not
# currently work on all systems.)
#
# On Windows, to specify the root directory of the bullet installation, set bullet_ROOT.
#
# Author: Siddhartha Chaudhuri, 2008
#

IF(WIN32)  # Windows

  SET(bullet_FOUND FALSE)

  IF(bullet_ROOT AND EXISTS "${bullet_ROOT}")
    SET(bullet_LIBRARY_DIRS ${bullet_ROOT}/lib)
    SET(bullet_LIBRARIES
        debug BulletCollision_d.lib
        debug BulletDynamics_d.lib
        debug BulletMultiThreaded_d.lib
        debug BulletSoftBody_d.lib
        debug LinearMath_d.lib
        optimized BulletCollision.lib
        optimized BulletDynamics.lib
        optimized BulletMultiThreaded.lib
        optimized BulletSoftBody.lib
        optimized LinearMath.lib)
    SET(bullet_LDFLAGS)
    IF(EXISTS ${bullet_ROOT}/include/bullet)
      SET(bullet_INCLUDE_DIRS ${bullet_ROOT}/include/bullet)
    ELSE()
      SET(bullet_INCLUDE_DIRS ${bullet_ROOT}/include)
    ENDIF()
    SET(bullet_CFLAGS)
    SET(bullet_FOUND TRUE)
  ENDIF(bullet_ROOT AND EXISTS "${bullet_ROOT}")

ELSE(WIN32)  # Linux etc
  FIND_PACKAGE(PkgConfig)
  IF(NOT PKG_CONFIG_FOUND)
    SET(PKG_CONFIG_CANDIDATE_PATH ${DEPENDENCIES_ROOT}/installed-pkg-config/bin)
    IF(EXISTS ${PKG_CONFIG_CANDIDATE_PATH})
      FIND_PROGRAM(PKG_CONFIG_EXECUTABLE NAMES pkg-config PATHS ${PKG_CONFIG_CANDIDATE_PATH} DOC "pkg-config executable")
      IF(PKG_CONFIG_EXECUTABLE)
        SET(PKG_CONFIG_FOUND 1)
      ENDIF(PKG_CONFIG_EXECUTABLE)
    ENDIF()
  ENDIF()
  IF(NOT PKG_CONFIG_FOUND)
    MESSAGE("Could not find pkg-config (to search for bullet)")
  ELSE(NOT PKG_CONFIG_FOUND)
    SET(ENV{PKG_CONFIG_PATH} ${bullet_ROOT}/lib/pkgconfig:${bullet_ROOT}/lib/pkgconfig)
    IF(bullet_MINIMUM_VERSION)
      PKG_CHECK_MODULES( bullet bullet>=${bullet_MINIMUM_VERSION})  # will set bullet_FOUND
    ELSE(bullet_MINIMUM_VERSION)
      PKG_CHECK_MODULES(bullet bullet)  # will set bullet_FOUND
    ENDIF(bullet_MINIMUM_VERSION)

  ENDIF(NOT PKG_CONFIG_FOUND)
  IF(bullet_FOUND)
  ENDIF()
ENDIF(WIN32)

IF(bullet_FOUND)
  IF(NOT bullet_FIND_QUIETLY)
    MESSAGE(STATUS "Found bullet: headers at ${bullet_INCLUDE_DIRS}, libraries at ${bullet_LIBRARY_DIRS}")
  ENDIF(NOT bullet_FIND_QUIETLY)
ELSE(bullet_FOUND)
  IF(bullet_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "bullet not found")
  ENDIF(bullet_FIND_REQUIRED)
ENDIF(bullet_FOUND)
#${TOP_LEVEL}/dependencies/bullet-2.78/src/.libs/libbulletcollision.so ${TOP_LEVEL}/dependencies/bullet-2.78/src/.libs/libbulletdynamics.so ${TOP_LEVEL}/dependencies/bullet-2.78/src/.libs/libbulletmath.so
