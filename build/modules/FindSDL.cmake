#Sirikata
#FindSDL.cmake
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

# Searches for an sdl installation
#
# Defines:
#
#   sdl_FOUND          True if sdl was found, else false
#   sdl_LIBRARIES      Libraries to link (without full path)
#   sdl_LIBRARY_DIRS   Directories containing the libraries (-L option)
#   sdl_LDFLAGS        All required linker flags
#   sdl_INCLUDE_DIRS   The directories containing header files (-I option)
#   sdl_CFLAGS         All required compiler flags
#
# If you set sdl_MINIMUM_VERSION, only a version equal to or higher than this will be searched for. (This feature does not
# currently work on all systems.)
#
# On Windows, to specify the root directory of the sdl installation, set sdl_ROOT.
#
# Author: Siddhartha Chaudhuri, 2008
#

IF(WIN32)  # Windows

  SET(sdl_FOUND FALSE)

  IF(sdl_ROOT AND EXISTS "${sdl_ROOT}")
    SET(sdl_LIBRARY_DIRS ${sdl_ROOT}/lib)
    SET(sdl_LIBRARIES
        debug sdl_d.lib
        optimized sdl.lib)
    SET(sdl_LDFLAGS)
    SET(sdl_INCLUDE_DIRS ${sdl_ROOT}/include ${sdl_ROOT}/samples/include)
    SET(sdl_CFLAGS)

    SET(sdl_FOUND TRUE)

  ENDIF(sdl_ROOT AND EXISTS "${sdl_ROOT}")

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
    MESSAGE("Could not find pkg-config (to search for sdl)")
  ELSE(NOT PKG_CONFIG_FOUND)
    SET(ENV{PKG_CONFIG_PATH} ${sdl_ROOT}/lib/pkgconfig:${sdl_ROOT}/../lib/pkgconfig)
    IF(sdl_MINIMUM_VERSION)
      PKG_CHECK_MODULES( sdl sdl>=${sdl_MINIMUM_VERSION})  # will set sdl_FOUND
    ELSE(sdl_MINIMUM_VERSION)
      PKG_CHECK_MODULES(sdl sdl)  # will set sdl_FOUND
    ENDIF(sdl_MINIMUM_VERSION)
    # MESSAGE(STATUS "SDL LIBRARIES: " ${sdl_LIBRARIES})
    # MESSAGE(STATUS "SDL LDFLAGS: " ${sdl_LDFLAGS})
    # SET(sdl_LIBRARIES "")
    # SET(sdl_LDFLAGS_ORIG ${sdl_LDFLAGS})
    # SET(sdl_LDFLAGS "")
    # FOREACH (LIB ${sdl_LDFLAGS_ORIG})
    #   IF(LIB STREQUAL "-lSDL")
    #   ELSE()
    #     SET(sdl_LDFLAGS ${sdl_LDFLAGS} ${LIB})
    #   ENDIF()
    # ENDFOREACH (LIB)
    # MESSAGE(STATUS "SDL LIBRARIES: " ${sdl_LIBRARIES})
    # MESSAGE(STATUS "SDL LDFLAGS: " ${sdl_LDFLAGS})
  ENDIF(NOT PKG_CONFIG_FOUND)
  IF(sdl_FOUND)
  ENDIF()
ENDIF(WIN32)

IF(sdl_FOUND)
  IF(NOT sdl_FIND_QUIETLY)
#    IF(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
#      MESSAGE(STATUS "Found sdl: ${sdl_LDFLAGS}")
#    ELSE(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    MESSAGE(STATUS "Found sdl: headers at ${sdl_INCLUDE_DIRS}, libraries at ${sdl_LIBRARY_DIRS}")
#    ENDIF(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
  ENDIF(NOT sdl_FIND_QUIETLY)
ELSE(sdl_FOUND)
  IF(sdl_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "sdl not found")
  ENDIF(sdl_FIND_REQUIRED)
ENDIF(sdl_FOUND)
