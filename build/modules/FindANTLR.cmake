# Try to find ANTLR include dirs and libraries
# Usage of this module is as follows:
# SET(ANTLR_ROOT /path/to/root)
# FIND_PACKAGE(ANTLR)
#
# Variables defined by this module:
#
#   ANTLR_FOUND              Set to true to indicate the library was found
#   ANTLR_INCLUDE_DIRS       The directory containing the ANTLR header files
#   ANTLR_LIBRARIES          The libraries needed to use ANTLR (without the full path)
#   ANTLR_COMPILER           The ANTLR compiler
# 
#Copyright (c) 2008, Patrick Reiter Horn
#FindANTLR.cmake
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
FIND_PATH(ANTLR_INCLUDE_DIRS NAMES antlr3string.h PATHS ${ANTLR_ROOT} ${ANTLR_ROOT}/include/ DOC "Location of ANTLR header files" NO_DEFAULT_PATH)
IF(NOT ANTLR_INCLUDE_DIRS)
    FIND_PATH(ANTLR_INCLUDE_DIRS NAMES antlr3string.h DOC "Location of antlr header files")
ENDIF()

SET(ANTLR_FOUND FALSE)

IF(ANTLR_INCLUDE_DIRS)
    # toplevel directory
    SET(ANTLR_ROOT_DIRS ${ANTLR_INCLUDE_DIRS})
    IF("${ANTLR_ROOT_DIRS}" MATCHES "/include$")
        # Destroy trailing "/include" in the path.
        GET_FILENAME_COMPONENT(ANTLR_ROOT_DIRS ${ANTLR_ROOT_DIRS} PATH)
    ENDIF()

    # compiler path
    SET(ANTLR_BIN_DIRS ${ANTLR_ROOT_DIRS})
    IF(EXISTS "${ANTLR_BIN_DIRS}/bin")
        SET(ANTLR_BIN_DIRS ${ANTLR_BIN_DIRS}/bin)
    ENDIF()
    # compiler inside binary directory
    FIND_FILE(ANTLR_COMPILER NAMES antlr-3.1.3.jar PATHS ${ANTLR_BIN_DIRS} NO_DEFAULT_PATH)

    # library path
    SET(ANTLR_LIBRARY_DIRS ${ANTLR_ROOT_DIRS})
    IF(EXISTS "${ANTLR_LIBRARY_DIRS}/lib")
        SET(ANTLR_LIBRARY_DIRS ${ANTLR_LIBRARY_DIRS}/lib)
    ENDIF(EXISTS "${ANTLR_LIBRARY_DIRS}/lib")

    IF(WIN32) # must distinguish debug and release builds

        FIND_LIBRARY(ANTLR_DEBUG_LIBRARY NAMES antlr3cd anltr3cd_dll libantlr3cd libanltr3cd_dll
                                 PATH_SUFFIXES "" Debug PATHS ${ANTLR_LIBRARY_DIRS} NO_DEFAULT_PATH)
        FIND_LIBRARY(ANTLR_RELEASE_LIBRARY NAMES antlr3c anltr3c_dll libantlr3c libanltr3c_dll
                                 PATH_SUFFIXES "" Release PATHS ${ANTLR_LIBRARY_DIRS} NO_DEFAULT_PATH)
        SET(ANTLR_LIBRARIES)
        IF(ANTLR_DEBUG_LIBRARY AND ANTLR_RELEASE_LIBRARY)
            SET(ANTLR_LIBRARIES debug ${ANTLR_DEBUG_LIBRARY} optimized ${ANTLR_RELEASE_LIBRARY})
        ELSEIF(ANTLR_DEBUG_LIBRARY)
            SET(ANTLR_LIBRARIES ${ANTLR_DEBUG_LIBRARY})
        ELSEIF(ANTLR_RELEASE_LIBRARY)
            SET(ANTLR_LIBRARIES ${ANTLR_RELEASE_LIBRARY})
        ENDIF()
    ELSE()
        FIND_LIBRARY(ANTLR_LIBRARIES NAMES antlr3c PATHS ${ANTLR_LIBRARY_DIRS} NO_DEFAULT_PATH)
    ENDIF()

    IF(ANTLR_LIBRARIES)
        SET(ANTLR_FOUND TRUE)
    ENDIF()
ENDIF()

IF(ANTLR_FOUND)
    IF(NOT ANTLR_FIND_QUIETLY)
        IF(ANTLR_COMPILER)
        MESSAGE(STATUS "ANTLR: includes at ${ANTLR_INCLUDE_DIRS}, libraries at ${ANTLR_LIBRARIES}, binary at ${ANTLR_COMPILER}")
        ELSE(ANTLR_COMPILER)
        MESSAGE(STATUS "ANTLR: includes at ${ANTLR_INCLUDE_DIRS}, libraries at ${ANTLR_LIBRARIES}")
        ENDIF(ANTLR_COMPILER)
    ENDIF()
ELSE()
    IF(ANTLR_FIND_REQUIRED)
        MESSAGE(FATAL_ERROR "ANTLR not found")
    ENDIF()
ENDIF()
