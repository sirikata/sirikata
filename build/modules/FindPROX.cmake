# Try to find PROX include dirs and libraries
# Usage of this module is as follows:
# SET(PROX_ROOT /path/to/root)
# FIND_PACKAGE(PROX)
#
# Variables defined by this module:
#
#   PROX_FOUND              Set to true to indicate the library was found
#   PROX_INCLUDE_DIRS       The directory containing the PROX header files
#   PROX_LIBRARIES          The libraries needed to use PROX (without the full path)
#   PROX_COMPILER           The PROX compiler
# 
#Copyright (c) 2008, Patrick Reiter Horn
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
# Look for the protocol buffers headers, first in the additional location and then in default system locations
IF(PROX_ROOT)
FIND_PATH(PROX_INCLUDE_DIRS NAMES prox/QueryHandler.hpp PATHS ${PROX_ROOT}/libprox/include/ ${PROX_ROOT}/include ${TOP_LEVEL}/externals/include ${TOP_LEVEL}/externals/prox/libprox/include/ ${TOP_LEVEL}/externals/include/ DOC "Location of PROX header files" NO_DEFAULT_PATH)
ELSE()
FIND_PATH(PROX_INCLUDE_DIRS NAMES prox/QueryHandler.hpp PATHS ${TOP_LEVEL}/externals/include ${TOP_LEVEL}/externals/prox/libprox/include/ ${TOP_LEVEL}/externals/include/ DOC "Location of PROX header files" NO_DEFAULT_PATH)
ENDIF()
IF(NOT PROX_INCLUDE_DIRS)
    FIND_PATH(PROX_INCLUDE_DIRS NAMES prox/QueryHandler.hpp DOC "Location of antlr header files")
ENDIF()

SET(PROX_FOUND FALSE)
IF(WIN32)
SET(PROX_CFLAGS -DNOMINMAX)
ELSE()
SET(PROX_CFLAGS)
ENDIF()
IF(PROX_INCLUDE_DIRS)
    # toplevel directory
    SET(PROX_ROOT_DIRS ${PROX_INCLUDE_DIRS})
    IF("${PROX_ROOT_DIRS}" MATCHES "/include$")
        # Destroy trailing "/include" in the path.
        GET_FILENAME_COMPONENT(PROX_ROOT_DIRS ${PROX_ROOT_DIRS} PATH)
    ENDIF()
    SET(PROX_LIBRARY_DIRS ${PROX_ROOT_DIRS})
    IF(EXISTS ${PROX_LIBRARY_DIRS}/src)
        SET(PROX_LIBRARY_DIRS ${PROX_LIBRARY_DIRS}/src)
    ENDIF()
    IF(PROX_LIBRARY_DIRS)
        SET(PROX_SOURCE_FILES
         ${PROX_LIBRARY_DIRS}/ArcAngle.cpp 
         ${PROX_LIBRARY_DIRS}/BruteForceQueryHandler.cpp 
         ${PROX_LIBRARY_DIRS}/Duration.cpp 
         ${PROX_LIBRARY_DIRS}/Object.cpp 
         ${PROX_LIBRARY_DIRS}/Quaternion.cpp 
         ${PROX_LIBRARY_DIRS}/QueryCache.cpp 
         ${PROX_LIBRARY_DIRS}/Query.cpp 
         ${PROX_LIBRARY_DIRS}/RTreeQueryHandler.cpp 
         ${PROX_LIBRARY_DIRS}/SolidAngle.cpp 
         ${PROX_LIBRARY_DIRS}/Time.cpp)
        SET(PROX_FOUND TRUE)
    ENDIF()
ENDIF()

IF(PROX_FOUND)
    IF(NOT PROX_FIND_QUIETLY)
        MESSAGE(STATUS "PROX: includes at ${PROX_INCLUDE_DIRS}, libraries at ${PROX_LIBRARY_DIRS}")
    ENDIF()
ELSE()
    IF(PROX_FIND_REQUIRED)
        MESSAGE(FATAL_ERROR "PROX not found")
    ENDIF()
ENDIF()
