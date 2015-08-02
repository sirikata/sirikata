# Find the thrift library.
#
# This module defines
#  THRIFT_FOUND             - True if THRIFT was found.
#  THRIFT_INCLUDE_DIRS      - Include directories for THRIFT headers.
#  THRIFT_LIBRARIES         - Libraries for THRIFT.
#
# To specify an additional directory to search, set THRIFT_ROOT.
#
# Copyright (c) 2010, Ewen Cheslack-Postava
# Based on FindSQLite3.cmake by:
#  Copyright (c) 2006, Jaroslaw Staniek, <js@iidea.pl>
#  Extended by Siddhartha Chaudhuri, 2008.
#
# Redistribution and use is allowed according to the terms of the BSD license.
#

SET(XZ_FOUND FALSE)
SET(XZ_INCLUDE_DIRS)
SET(XZ_LIBRARIES)

IF(XZ_ROOT)
  SET(SEARCH_PATHS
    ${XZ_ROOT}/
    ${XZ_ROOT}/include
    ${SEARCH_PATHS}
    )
ENDIF()

FIND_PATH(XZ_INCLUDE_DIRS
  NAMES lzma.h
  PATHS ${SEARCH_PATHS}
  NO_DEFAULT_PATH)
IF(NOT XZ_INCLUDE_DIRS)  # now look in system locations
  FIND_PATH(XZ_INCLUDE_DIRS NAMES lzma.h)
ENDIF(NOT XZ_INCLUDE_DIRS)
SET(XZ_LIBRARY_DIRS)
IF(XZ_ROOT)
  SET(XZ_LIBRARY_DIRS ${XZ_ROOT})
  IF(EXISTS "${XZ_ROOT}/lib")
    SET(XZ_LIBRARY_DIRS ${XZ_LIBRARY_DIRS} ${XZ_ROOT}/lib)
  ENDIF()
ENDIF()

# Without system dirs
FIND_LIBRARY(XZ_LIBRARY
  NAMES lzma
  PATHS ${XZ_LIBRARY_DIRS}
  NO_DEFAULT_PATH
  )
IF(NOT XZ_LIBRARY)  # now look in system locations
  FIND_LIBRARY(XZ_LIBRARY NAMES xz)
ENDIF(NOT XZ_LIBRARY)

SET(XZ_LIBRARIES)
IF(XZ_LIBRARY)
  SET(XZ_LIBRARIES ${XZ_LIBRARY})
ENDIF()

IF(XZ_INCLUDE_DIRS AND XZ_LIBRARIES)
  SET(XZ_FOUND TRUE)
  IF(NOT XZ_FIND_QUIETLY)
    MESSAGE(STATUS "Found xz: headers at ${XZ_INCLUDE_DIRS}, libraries at ${XZ_LIBRARY_DIRS} :: ${XZ_LIBRARIES}")
  ENDIF(NOT XZ_FIND_QUIETLY)
ELSE(XZ_INCLUDE_DIRS AND XZ_LIBRARIES)
  SET(XZ_FOUND FALSE)
  IF(XZ_FIND_REQUIRED)
    MESSAGE(STATUS "xz not found")
  ENDIF(XZ_FIND_REQUIRED)
ENDIF(XZ_INCLUDE_DIRS AND XZ_LIBRARIES)

MARK_AS_ADVANCED(XZ_INCLUDE_DIRS XZ_LIBRARIES)
