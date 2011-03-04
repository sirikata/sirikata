#Sirikata
#FindOGRE.cmake
#
#Copyright (c) 2009, Daniel Reiter Horn
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

# Searches for an OGRE installation
#
# Defines:
#
#   OGRE_FOUND          True if OGRE was found, else false
#   OGRE_LIBRARIES      Libraries to link (without full path)
#   OGRE_LIBRARY_DIRS   Directories containing the libraries (-L option)
#   OGRE_LDFLAGS        All required linker flags
#   OGRE_INCLUDE_DIRS   The directories containing header files (-I option)
#   OGRE_CFLAGS         All required compiler flags
#
# If you set OGRE_MINIMUM_VERSION, only a version equal to or higher than this will be searched for. (This feature does not
# currently work on all systems.)
#
# On Windows, to specify the root directory of the OGRE installation, set OGRE_ROOT.
#
# Author: Siddhartha Chaudhuri, 2008
#

IF(WIN32)  # Windows

  SET(OGRE_FOUND FALSE)

  IF(OGRE_ROOT AND EXISTS "${OGRE_ROOT}")
    SET(OGRE_LIBRARY_DIRS ${OGRE_ROOT}/lib/Release ${OGRE_ROOT}/lib/Debug ${OGRE_ROOT}/lib)
    SET(OGRE_LIBRARIES
        debug OgreMain_d.lib
        optimized OgreMain.lib)
    SET(OGRE_LDFLAGS)
    SET(OGRE_INCLUDE_DIRS ${OGRE_ROOT}/include ${OGRE_ROOT}/samples/include)
    SET(OGRE_CFLAGS)

    SET(OGRE_FOUND TRUE)
    SET(OGRE_ZIP_PATH ${OGRE_ROOT}/data/OgreCore.zip)

  ENDIF(OGRE_ROOT AND EXISTS "${OGRE_ROOT}")

ELSEIF(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")  # OS X

  SET(OGRE_FOUND FALSE)

  #INCLUDE(CMakeFindFrameworks)
  #CMAKE_FIND_FRAMEWORKS(OGRE) 
  SET(OGRE_FRAMEWORKS ${OGRE_ROOT}/Frameworks/Ogre.framework)
  IF(EXISTS ${OGRE_FRAMEWORKS})
    LIST(GET OGRE_FRAMEWORKS 0 OGRE_LIBRARIES)
    SET(OGRE_INCLUDE_DIRS ${OGRE_LIBRARIES}/Headers)
    
    # Unset other variables
    SET(OGRE_LDFLAGS -F${OGRE_LIBRARIES}/.. -framework Ogre)
    SET(OGRE_ZIP_PATH ${OGRE_LIBRARIES}/Resources/OgreCore.zip)

    SET(OGRE_LIBRARY_DIRS)
    SET(OGRE_LIBRARIES)
    SET(OGRE_CFLAGS)
    SET(OGRE_FOUND TRUE)
  ENDIF(EXISTS ${OGRE_FRAMEWORKS})

ELSE(WIN32)  # Linux etc
  FIND_PACKAGE(PkgConfig)
  IF(NOT PKG_CONFIG_FOUND)
    MESSAGE("Could not find pkg-config (to search for OGRE)")
  ELSE(NOT PKG_CONFIG_FOUND)
    SET(ENV{PKG_CONFIG_PATH} ${OGRE_ROOT}/../lib/pkgconfig)
    IF(OGRE_MINIMUM_VERSION)
      PKG_CHECK_MODULES( OGRE OGRE>=${OGRE_MINIMUM_VERSION})  # will set OGRE_FOUND
    ELSE(OGRE_MINIMUM_VERSION)
      PKG_CHECK_MODULES(OGRE OGRE)  # will set OGRE_FOUND
    ENDIF(OGRE_MINIMUM_VERSION)
  ENDIF(NOT PKG_CONFIG_FOUND)
  IF(OGRE_FOUND)
    SET(OGRE_ZIP_PATH ${OGRE_ROOT}/Samples/Media/packs/OgreCore.zip)
  ENDIF()
ENDIF(WIN32)

IF(OGRE_FOUND)
  IF(EXISTS ${OGRE_ZIP_PATH})
      IF(NOT OGRE_FIND_QUIETLY)
          MESSAGE(STATUS "OgreCore.zip: ${OGRE_ZIP_PATH}")
      ENDIF()
  ELSE()
      IF(NOT OGRE_FIND_QUIETLY)
          MESSAGE(STATUS "Failed to find ogre at ${OGRE_ZIP_PATH}")
      ENDIF()
      
      SET(OGRE_ZIP_PATH)
  ENDIF()
  IF(NOT OGRE_FIND_QUIETLY)
    IF(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
      MESSAGE(STATUS "Found OGRE: ${OGRE_LDFLAGS}")
    ELSE(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
      MESSAGE(STATUS "Found OGRE: headers at ${OGRE_INCLUDE_DIRS}, libraries at ${OGRE_LIBRARY_DIRS}")
    ENDIF(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
  ENDIF(NOT OGRE_FIND_QUIETLY)
ELSE(OGRE_FOUND)
  IF(OGRE_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "OGRE not found")
  ENDIF(OGRE_FIND_REQUIRED)
ENDIF(OGRE_FOUND)
