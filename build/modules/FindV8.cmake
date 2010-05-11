# Find the V8 Javascript library.
#
# This module defines
#  V8_FOUND             - True if V8 was found.
#  V8_INCLUDE_DIRS      - Include directories for V8 headers.
#  V8_LIBRARIES         - Libraries for V8.
#
# To specify an additional directory to search, set V8_ROOT.
#
# Copyright (c) 2010, Ewen Cheslack-Postava
# Based on FindSQLite3.cmake by:
#  Copyright (c) 2006, Jaroslaw Staniek, <js@iidea.pl>
#  Extended by Siddhartha Chaudhuri, 2008.
#
# Redistribution and use is allowed according to the terms of the BSD license.
#

SET(V8_FOUND FALSE)
SET(V8_INCLUDE_DIRS)
SET(V8_LIBRARIES)

SET(SEARCH_PATHS
  $ENV{ProgramFiles}/v8/include
  $ENV{SystemDrive}/v8/include
  $ENV{ProgramFiles}/v8
  $ENV{SystemDrive}/v8
  )
IF(V8_ROOT)
  SET(SEARCH_PATHS
    ${V8_ROOT}
    ${V8_ROOT}/include
    ${SEARCH_PATHS}
    )
ENDIF()

FIND_PATH(V8_INCLUDE_DIRS
  NAMES v8.h
  PATHS ${SEARCH_PATHS}
  NO_DEFAULT_PATH)
IF(NOT V8_INCLUDE_DIRS)  # now look in system locations
  FIND_PATH(V8_INCLUDE_DIRS NAMES v8.h)
ENDIF(NOT V8_INCLUDE_DIRS)

SET(V8_LIBRARY_DIRS)
IF(V8_ROOT)
  SET(V8_LIBRARY_DIRS ${V8_ROOT})
  IF(EXISTS "${V8_ROOT}/lib")
    SET(V8_LIBRARY_DIRS ${V8_LIBRARY_DIRS} ${V8_ROOT}/lib)
  ENDIF()
ENDIF()

# Without system dirs
FIND_LIBRARY(V8_LIBRARIES
  NAMES v8
  PATHS ${V8_LIBRARY_DIRS}
  NO_DEFAULT_PATH
  )
IF(NOT V8_LIBRARIES)  # now look in system locations
  FIND_LIBRARY(V8_LIBRARIES
    NAMES v8
    )
ENDIF(NOT V8_LIBRARIES)

IF(V8_INCLUDE_DIRS AND V8_LIBRARIES)
  SET(V8_FOUND TRUE)
  IF(NOT V8_FIND_QUIETLY)
    MESSAGE(STATUS "Found V8: headers at ${V8_INCLUDE_DIRS}, libraries at ${V8_LIBRARIES}")
  ENDIF(NOT V8_FIND_QUIETLY)
ELSE(V8_INCLUDE_DIRS AND V8_LIBRARIES)
  SET(V8_FOUND FALSE)
  IF(V8_FIND_REQUIRED)
    MESSAGE(STATUS "V8 not found")
  ENDIF(V8_FIND_REQUIRED)
ENDIF(V8_INCLUDE_DIRS AND V8_LIBRARIES)

MARK_AS_ADVANCED(V8_INCLUDE_DIRS V8_LIBRARIES)
