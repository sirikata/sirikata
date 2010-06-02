#Sirikata
#FindOIS.cmake
#
#Copyright (c) 2008, Siddhartha Chaudhuri
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

# Searches for an OIS installation
#
# Defines:
#
#   OIS_FOUND         True if OIS was found, else false
#   OIS_LIBRARIES     Libraries to link
#   OIS_INCLUDE_DIRS  The directories containing the header files
#
# To specify an additional directory to search, set OIS_ROOT.
#
# Author: Siddhartha Chaudhuri, 2008
#

SET(OIS_FOUND FALSE)

IF(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")  # OS X

  SET(OIS_FRAMEWORKS ${OIS_ROOT}/Frameworks/OIS.framework)
  IF(EXISTS ${OIS_FRAMEWORKS})
    LIST(GET OIS_FRAMEWORKS 0 OIS_LIBRARIES)
    SET(OIS_INCLUDE_DIRS ${OIS_LIBRARIES}/Headers)
    SET(OIS_LDFLAGS -F${OIS_LIBRARIES}/Frameworks -framework OIS)
    SET(OIS_LIBRARY_DIRS)
    SET(OIS_LIBRARIES)
    SET(OIS_CFLAGS)

    SET(OIS_FOUND TRUE)
  ENDIF(EXISTS ${OIS_FRAMEWORKS})

ELSE(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")  # Windows, Linux etc

  SET(OIS_INCLUDE_DOC "The directory containing OIS/OIS.h")
  FIND_PATH(OIS_INCLUDE_DIRS NAMES OIS/OIS.h PATHS ${OIS_ROOT} ${OIS_ROOT}/include DOC ${OIS_INCLUDE_DOC} NO_DEFAULT_PATH)
  IF(NOT OIS_INCLUDE_DIRS)  # now look in system locations
    FIND_PATH(OIS_INCLUDE_DIRS NAMES OIS/OIS.h DOC ${OIS_INCLUDE_DOC})
  ENDIF(NOT OIS_INCLUDE_DIRS)

  IF(OIS_INCLUDE_DIRS)
    SET(OIS_LIBRARY_DIRS ${OIS_INCLUDE_DIRS})
    IF("${OIS_LIBRARY_DIRS}" MATCHES "/include$")
      # Strip off the trailing "/include" in the path.
      GET_FILENAME_COMPONENT(OIS_LIBRARY_DIRS ${OIS_LIBRARY_DIRS} PATH)
    ENDIF("${OIS_LIBRARY_DIRS}" MATCHES "/include$")

    IF(EXISTS "${OIS_LIBRARY_DIRS}/lib")
      SET(OIS_LIBRARY_DIRS ${OIS_LIBRARY_DIRS}/lib)
    ENDIF(EXISTS "${OIS_LIBRARY_DIRS}/lib")

    IF(WIN32)  # Windows
      FIND_LIBRARY(OIS_DEBUG_LIBRARY   NAMES OISd OIS_d libOISd libOIS_d
                   PATH_SUFFIXES "" Debug   PATHS ${OIS_LIBRARY_DIRS} NO_DEFAULT_PATH)
      FIND_LIBRARY(OIS_RELEASE_LIBRARY NAMES OIS libOIS
                   PATH_SUFFIXES "" Release PATHS ${OIS_LIBRARY_DIRS} NO_DEFAULT_PATH)

      SET(OIS_LIBRARIES)
      IF(OIS_DEBUG_LIBRARY AND OIS_RELEASE_LIBRARY)
        SET(OIS_LIBRARIES debug ${OIS_DEBUG_LIBRARY} optimized ${OIS_RELEASE_LIBRARY})
      ELSEIF(OIS_DEBUG_LIBRARY)
        SET(OIS_LIBRARIES ${OIS_DEBUG_LIBRARY})
      ELSEIF(OIS_RELEASE_LIBRARY)
        SET(OIS_LIBRARIES ${OIS_RELEASE_LIBRARY})
      ENDIF(OIS_DEBUG_LIBRARY AND OIS_RELEASE_LIBRARY)
    ELSE(WIN32)  # Linux etc
      FIND_LIBRARY(OIS_LIBRARIES NAMES OIS PATHS ${OIS_LIBRARY_DIRS} NO_DEFAULT_PATH)
    ENDIF(WIN32)

    IF(OIS_LIBRARIES)
      SET(OIS_FOUND TRUE)
    ENDIF(OIS_LIBRARIES)
  ENDIF(OIS_INCLUDE_DIRS)

ENDIF(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")

IF(OIS_FOUND)
  IF(NOT OIS_FIND_QUIETLY)
    IF(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
      MESSAGE(STATUS "Found OIS: ${OIS_LDFLAGS}")
    ELSE(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
      MESSAGE(STATUS "Found OIS: headers at ${OIS_INCLUDE_DIRS}, libraries at ${OIS_LIBRARIES}")
    ENDIF(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
  ENDIF(NOT OIS_FIND_QUIETLY)
ELSE(OIS_FOUND)
  IF(OIS_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "OIS not found")
  ENDIF(OIS_FIND_REQUIRED)
ENDIF(OIS_FOUND)
