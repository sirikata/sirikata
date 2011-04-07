#Sirikata
#FindBreakpad.cmake
#
# Converted from SQLite -> Breakpad by Ewen Cheslack-Postava, 2011
# Copyright (c) 2006, Jaroslaw Staniek, <js@iidea.pl>
# Extended by Siddhartha Chaudhuri, 2008.
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


# Find the Breakpad includes and library
#
# This module defines
#  BREAKPAD_INCLUDE_DIRS     - Where to find the Breakpad header
#  BREAKPAD_LIBRARIES        - The libraries needed to use Breakpad
#  BREAKPAD_LIBRARY_DIRS   Directories containing the libraries (-L option)
#  BREAKPAD_FOUND            - If false, do not try to use Breakpad
#
# To specify an additional directory to search, set BREAKPAD_ROOT.
#
# Copyright (c) 2006, Jaroslaw Staniek, <js@iidea.pl>
# Extended by Siddhartha Chaudhuri, 2008.
#
# Redistribution and use is allowed according to the terms of the BSD license.
#

SET(BREAKPAD_FOUND FALSE)

IF(BREAKPAD_INCLUDE_DIRS AND BREAKPAD_LIBRARIES)

   SET(BREAKPAD_FOUND TRUE)

ELSEIF(WIN32)

  FIND_PATH(BREAKPAD_INCLUDE_DIRS
            NAMES client/windows/handler/exception_handler.h
            PATHS
            ${BREAKPAD_ROOT}
            ${BREAKPAD_ROOT}/include
            NO_DEFAULT_PATH)
  IF(NOT BREAKPAD_INCLUDE_DIRS)  # now look in system locations
    FIND_PATH(BREAKPAD_INCLUDE_DIRS NAMES client/windows/handler/exception_handler.h)
  ENDIF(NOT BREAKPAD_INCLUDE_DIRS)

  IF(BREAKPAD_ROOT)
    SET(BREAKPAD_LIBRARY_DIRS ${BREAKPAD_ROOT})
    IF(EXISTS "${BREAKPAD_LIBRARY_DIRS}/lib")
        SET(BREAKPAD_LIBRARY_DIRS ${BREAKPAD_LIBRARY_DIRS} ${BREAKPAD_LIBRARY_DIRS}/lib)
    ENDIF()

    FIND_LIBRARY(BREAKPAD_EXCEPTION_HANDLER_LIBRARY_DEBUG
                 NAMES exception_handler_d
                 PATHS
                 ${BREAKPAD_LIBRARY_DIRS}
                 NO_DEFAULT_PATH)
    FIND_LIBRARY(BREAKPAD_EXCEPTION_HANDLER_LIBRARY_RELEASE
                 NAMES exception_handler
                 PATHS
                 ${BREAKPAD_LIBRARY_DIRS}
                 NO_DEFAULT_PATH)

    FIND_LIBRARY(BREAKPAD_COMMON_LIBRARY_DBUG
                 NAMES common_d
                 PATHS
                 ${BREAKPAD_LIBRARY_DIRS}
                 NO_DEFAULT_PATH)
    FIND_LIBRARY(BREAKPAD_COMMON_LIBRARY_RELEASE
                 NAMES common
                 PATHS
                 ${BREAKPAD_LIBRARY_DIRS}
                 NO_DEFAULT_PATH)

    FIND_LIBRARY(BREAKPAD_CRASH_GEN_CLIENT_LIBRARY_DEBUG
                 NAMES crash_generation_client_d
                 PATHS
                 ${BREAKPAD_LIBRARY_DIRS}
                 NO_DEFAULT_PATH)
    FIND_LIBRARY(BREAKPAD_CRASH_GEN_CLIENT_LIBRARY_RELEASE
                 NAMES crash_generation_client
                 PATHS
                 ${BREAKPAD_LIBRARY_DIRS}
                 NO_DEFAULT_PATH)
  ENDIF()
  
  IF(NOT BREAKPAD_COMMON_LIBRARY_RELEASE OR NOT BREAKPAD_EXCEPTION_HANDLER_LIBRARY_RELEASE OR NOT BREAKPAD_CRASH_GEN_CLIENT_LIBRARY_RELEASE)  # now look in system locations
    FIND_LIBRARY(BREAKPAD_COMMON_LIBRARY_DEBUG NAMES common_d)
    FIND_LIBRARY(BREAKPAD_COMMON_LIBRARY_RELEASE NAMES common)
    FIND_LIBRARY(BREAKPAD_EXCEPTION_HANDLER_LIBRARY_DEBUG NAMES exception_handler_d)
    FIND_LIBRARY(BREAKPAD_EXCEPTION_HANDLER_LIBRARY_RELEASE NAMES exception_handler)
    FIND_LIBRARY(BREAKPAD_CRASH_GEN_CLIENT_LIBRARY_DEBUG NAMES crash_generation_client_d)
    FIND_LIBRARY(BREAKPAD_CRASH_GEN_CLIENT_LIBRARY_RELEASE NAMES crash_generation_client)
  ENDIF()

  IF(BREAKPAD_COMMON_LIBRARY_RELEASE AND BREAKPAD_EXCEPTION_HANDLER_LIBRARY_RELEASE AND BREAKPAD_CRASH_GEN_CLIENT_LIBRARY_RELEASE)
    SET(BREAKPAD_LIBRARIES debug "wininet" ${BREAKPAD_COMMON_LIBRARY_DEBUG} ${BREAKPAD_EXCEPTION_HANDLER_LIBRARY_DEBUG} ${BREAKPAD_CRASH_GEN_CLIENT_LIBRARY_DEBUG} optimized "wininet" ${BREAKPAD_COMMON_LIBRARY_RELEASE} ${BREAKPAD_EXCEPTION_HANDLER_LIBRARY_RELEASE} ${BREAKPAD_CRASH_GEN_CLIENT_LIBRARY_RELEASE})
  ENDIF()

  IF(BREAKPAD_INCLUDE_DIRS AND BREAKPAD_LIBRARIES)
    SET(BREAKPAD_FOUND TRUE)
  ELSE(BREAKPAD_INCLUDE_DIRS AND BREAKPAD_LIBRARIES)
    SET(BREAKPAD_FOUND FALSE)
  ENDIF(BREAKPAD_INCLUDE_DIRS AND BREAKPAD_LIBRARIES)

  MARK_AS_ADVANCED(BREAKPAD_INCLUDE_DIRS BREAKPAD_LIBRARIES)

ELSE()

  # No support on linux, mac yet
  SET(BREAKPAD_FOUND FALSE)

ENDIF(BREAKPAD_INCLUDE_DIRS AND BREAKPAD_LIBRARIES)

# Report results
IF(NOT BREAKPAD_FIND_QUIETLY)
  MESSAGE(STATUS "Found Beakpad: headers at ${BREAKPAD_INCLUDE_DIRS}, libraries at ${BREAKPAD_LIBRARIES}")
ELSE()
  IF(BREAKPAD_FIND_REQUIRED)
    MESSAGE(STATUS "BREAKPAD not found")
  ENDIF(BREAKPAD_FIND_REQUIRED)
ENDIF()
