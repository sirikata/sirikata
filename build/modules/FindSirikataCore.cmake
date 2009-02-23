# Searches for an installation of the Sirikata core library. On success, it sets the following variables:
#
#   SirikataCore_FOUND              Set to true to indicate the Sirikata core library was found
#   SirikataCore_INCLUDE_DIRS       The directory containing the header file sirikata/util/Platform.hpp
#   SirikataCore_LIBRARIES          The libraries needed to use the Sirikata core library
#
# To specify an additional directory to search, set SirikataCore_ROOT.
#
# Copyright (C) Siddhartha Chaudhuri, 2009
#

# Look for the header, first in the user-specified location and then in the system locations
SET(SirikataCore_INCLUDE_DOC "The directory containing the header file sirikata/util/Platform.hpp")
FIND_PATH(SirikataCore_INCLUDE_DIRS NAMES sirikata/util/Platform.hpp PATHS ${SirikataCore_ROOT} ${SirikataCore_ROOT}/include
          DOC ${SirikataCore_INCLUDE_DOC} NO_DEFAULT_PATH)
IF(NOT SirikataCore_INCLUDE_DIRS)  # now look in system locations
  FIND_PATH(SirikataCore_INCLUDE_DIRS NAMES sirikata/util/Platform.hpp DOC ${SirikataCore_INCLUDE_DOC})
ENDIF(NOT SirikataCore_INCLUDE_DIRS)

SET(SirikataCore_FOUND FALSE)

IF(SirikataCore_INCLUDE_DIRS)
  SET(SirikataCore_LIBRARY_DIRS ${SirikataCore_INCLUDE_DIRS})

  IF("${SirikataCore_LIBRARY_DIRS}" MATCHES "/include$")
    # Strip off the trailing "/include" in the path.
    GET_FILENAME_COMPONENT(SirikataCore_LIBRARY_DIRS ${SirikataCore_LIBRARY_DIRS} PATH)
  ENDIF("${SirikataCore_LIBRARY_DIRS}" MATCHES "/include$")

  IF(EXISTS "${SirikataCore_LIBRARY_DIRS}/lib")
    SET(SirikataCore_LIBRARY_DIRS ${SirikataCore_LIBRARY_DIRS}/lib)
  ENDIF(EXISTS "${SirikataCore_LIBRARY_DIRS}/lib")

  # Find SirikataCore libraries
  FIND_LIBRARY(SirikataCore_DEBUG_LIBRARY   NAMES sirikata-core_d sirikata-cored libsirikata-core_d libsirikata-cored
               PATH_SUFFIXES "" Debug   PATHS ${SirikataCore_LIBRARY_DIRS} NO_DEFAULT_PATH)
  FIND_LIBRARY(SirikataCore_RELEASE_LIBRARY NAMES sirikata-core libsirikata-core
               PATH_SUFFIXES "" Release PATHS ${SirikataCore_LIBRARY_DIRS} NO_DEFAULT_PATH)

  SET(SirikataCore_LIBRARIES )
  IF(SirikataCore_DEBUG_LIBRARY AND SirikataCore_RELEASE_LIBRARY)
    SET(SirikataCore_LIBRARIES debug ${SirikataCore_DEBUG_LIBRARY} optimized ${SirikataCore_RELEASE_LIBRARY})
  ELSEIF(SirikataCore_DEBUG_LIBRARY)
    SET(SirikataCore_LIBRARIES ${SirikataCore_DEBUG_LIBRARY})
  ELSEIF(SirikataCore_RELEASE_LIBRARY)
    SET(SirikataCore_LIBRARIES ${SirikataCore_RELEASE_LIBRARY})
  ENDIF(SirikataCore_DEBUG_LIBRARY AND SirikataCore_RELEASE_LIBRARY)

  IF(SirikataCore_LIBRARIES)
    SET(SirikataCore_FOUND TRUE)
  ENDIF(SirikataCore_LIBRARIES)
ENDIF(SirikataCore_INCLUDE_DIRS)

IF(SirikataCore_FOUND)
  IF(NOT SirikataCore_FIND_QUIETLY)
    MESSAGE(STATUS "Found Sirikata-Core: headers at ${SirikataCore_INCLUDE_DIRS}, libraries at ${SirikataCore_LIBRARY_DIRS}")
  ENDIF(NOT SirikataCore_FIND_QUIETLY)
ELSE(SirikataCore_FOUND)
  IF(SirikataCore_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "Sirikata-Core not found")
  ENDIF(SirikataCore_FIND_REQUIRED)
ENDIF(SirikataCore_FOUND)
