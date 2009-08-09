# Find the SQLite includes and library
#
# This module defines
#  SQLite3_INCLUDE_DIRS     - Where to find the SQLite3 header
#  SQLite3_LIBRARIES        - The libraries needed to use SQLite3
#  SQLite3_LIBRARY_DIRS   Directories containing the libraries (-L option)
#  SQLite3_FOUND            - If false, do not try to use SQlite3
#  SQLite3_MAJOR_VERSION    - The major version of the library (e.g. 3 for 3.4.1)
#  SQLite3_MINOR_VERSION    - The minor version of the library (e.g. 4 for 3.4.1)
#  SQLite3_SUBMINOR_VERSION - The subminor version of the library (e.g. 1 for 3.4.1)
#
# To specify an additional directory to search, set SQLite3_ROOT.
#
# Copyright (c) 2006, Jaroslaw Staniek, <js@iidea.pl>
# Extended by Siddhartha Chaudhuri, 2008.
#
# Redistribution and use is allowed according to the terms of the BSD license.
#

IF(SQLite3_INCLUDE_DIRS AND SQLite3_LIBRARIES)

   SET(SQLite3_FOUND TRUE)

ELSE(SQLite3_INCLUDE_DIRS AND SQLite3_LIBRARIES)

  FIND_PATH(SQLite3_INCLUDE_DIRS
            NAMES sqlite3.h
            PATHS
            ${SQLite3_ROOT}
            ${SQLite3_ROOT}/include
            $ENV{ProgramFiles}/SQLite/include
            $ENV{SystemDrive}/SQLite/include
            $ENV{ProgramFiles}/SQLite
            $ENV{SystemDrive}/SQLite
            $ENV{ProgramFiles}/SQLite3/include
            $ENV{SystemDrive}/SQLite3/include
            $ENV{ProgramFiles}/SQLite3
            $ENV{SystemDrive}/SQLite3
            NO_DEFAULT_PATH)
  IF(NOT SQLite3_INCLUDE_DIRS)  # now look in system locations
    FIND_PATH(SQLite3_INCLUDE_DIRS NAMES sqlite3.h)
  ENDIF(NOT SQLite3_INCLUDE_DIRS)
IF(SQLite3_ROOT)
  SET(SQLite3_LIBRARY_DIRS ${SQLite3_ROOT})
  IF(EXISTS "${SQLite3_LIBRARY_DIRS}/lib")
      SET(SQLite3_LIBRARY_DIRS ${SQLite3_LIBRARY_DIRS} ${SQLite3_LIBRARY_DIRS}/lib)
  ENDIF()
  FIND_LIBRARY(SQLite3_LIBRARIES
               NAMES sqlite3
               PATHS
               ${SQLite3_LIBRARY_DIRS}
               NO_DEFAULT_PATH)
ENDIF()
  IF(NOT SQLite3_LIBRARIES)  # now look in system locations
    FIND_LIBRARY(SQLite3_LIBRARIES NAMES sqlite3)
  ENDIF(NOT SQLite3_LIBRARIES)

  IF(SQLite3_INCLUDE_DIRS AND SQLite3_LIBRARIES)
    SET(SQLite3_FOUND TRUE)
    IF(NOT SQLite3_FIND_QUIETLY)
      MESSAGE(STATUS "Found SQLite3: headers at ${SQLite3_INCLUDE_DIRS}, libraries at ${SQLite3_LIBRARIES}")
    ENDIF(NOT SQLite3_FIND_QUIETLY)
  ELSE(SQLite3_INCLUDE_DIRS AND SQLite3_LIBRARIES)
    SET(SQLite3_FOUND FALSE)
    IF(SQLite3_FIND_REQUIRED)
      MESSAGE(STATUS "SQLite3 not found")
    ENDIF(SQLite3_FIND_REQUIRED)
  ENDIF(SQLite3_INCLUDE_DIRS AND SQLite3_LIBRARIES)

  MARK_AS_ADVANCED(SQLite3_INCLUDE_DIRS SQLite3_LIBRARIES)

ENDIF(SQLite3_INCLUDE_DIRS AND SQLite3_LIBRARIES)

# Extract the library version
IF(SQLite3_FOUND)
  SET(SQLITE_VERSION_HEADER ${SQLite3_INCLUDE_DIRS}/sqlite3.h)
  IF(EXISTS ${SQLITE_VERSION_HEADER})
    SET(SQLite3_VERSION)
    FILE(READ ${SQLITE_VERSION_HEADER} _SQLite3_H_CONTENTS)

    STRING(REGEX MATCH "#define[ \t]+SQLITE_VERSION_NUMBER[ \t]+[0-9]+" SQLite3_VERSION "${_SQLite3_H_CONTENTS}")
    STRING(REGEX MATCH "[0-9]+" SQLite3_VERSION ${SQLite3_VERSION})

    SET(SQLite3_VERSION ${SQLite3_VERSION} CACHE INTERNAL "The version number for the SQLite3 library")

    IF(SQLite3_VERSION)
      MATH(EXPR SQLite3_MAJOR_VERSION "${SQLite3_VERSION} / 1000000")
      MATH(EXPR SQLite3_MINOR_VERSION "${SQLite3_VERSION} / 1000 % 1000")
      MATH(EXPR SQLite3_SUBMINOR_VERSION "${SQLite3_VERSION} % 1000")
    ENDIF(SQLite3_VERSION)
  ELSE()
    SET(SQLite3_FOUND FALSE)
  ENDIF()
ENDIF(SQLite3_FOUND)
