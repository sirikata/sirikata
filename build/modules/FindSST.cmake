# Searches for an installation of the SST core library. On success, it sets the following variables:
#
#   SST_FOUND              Set to true to indicate the Sirikata core library was found
#   SST_INCLUDE_DIRS       The directory containing the header file sst/stream.h
#   SST_LIBRARIES          The libraries needed to use the Sirikata core library
#
# To specify an additional directory to search, set SST_ROOT.
#
# Copyright (C) Siddhartha Chaudhuri, 2009
# Copyright (C) Ewen Cheslack-Postava, 2009
#

# Look for the header, first in the user-specified location and then in the system locations
SET(SST_INCLUDE_DOC "The directory containing the header file sst/stream.h")
FIND_PATH(SST_INCLUDE_DIRS NAMES sst/stream.h PATHS ${SST_ROOT} ${SST_ROOT}/include
          DOC ${SST_INCLUDE_DOC} NO_DEFAULT_PATH)
IF(NOT SST_INCLUDE_DIRS)  # now look in system locations
  FIND_PATH(SST_INCLUDE_DIRS NAMES sst/stream.h DOC ${SST_INCLUDE_DOC})
ENDIF(NOT SST_INCLUDE_DIRS)

SET(SST_FOUND FALSE)

IF(SST_INCLUDE_DIRS)
  SET(SST_LIBRARY_DIRS ${SST_INCLUDE_DIRS})

  IF("${SST_LIBRARY_DIRS}" MATCHES "/include$")
    # Strip off the trailing "/include" in the path.
    GET_FILENAME_COMPONENT(SST_LIBRARY_DIRS ${SST_LIBRARY_DIRS} PATH)
  ENDIF("${SST_LIBRARY_DIRS}" MATCHES "/include$")

  IF(EXISTS "${SST_LIBRARY_DIRS}/lib")
    SET(SST_LIBRARY_DIRS ${SST_LIBRARY_DIRS}/lib)
  ENDIF(EXISTS "${SST_LIBRARY_DIRS}/lib")

  # Find SST libraries
  FIND_LIBRARY(SST_DEBUG_LIBRARY   NAMES sst_d sstd libsst_d libsstd
               PATH_SUFFIXES "" Debug   PATHS ${SST_LIBRARY_DIRS} NO_DEFAULT_PATH)
  FIND_LIBRARY(SST_RELEASE_LIBRARY NAMES sst libsst
               PATH_SUFFIXES "" Release PATHS ${SST_LIBRARY_DIRS} NO_DEFAULT_PATH)

  SET(SST_LIBRARIES )
  IF(SST_DEBUG_LIBRARY AND SST_RELEASE_LIBRARY)
    SET(SST_LIBRARIES debug ${SST_DEBUG_LIBRARY} optimized ${SST_RELEASE_LIBRARY})
  ELSEIF(SST_DEBUG_LIBRARY)
    SET(SST_LIBRARIES ${SST_DEBUG_LIBRARY})
  ELSEIF(SST_RELEASE_LIBRARY)
    SET(SST_LIBRARIES ${SST_RELEASE_LIBRARY})
  ENDIF(SST_DEBUG_LIBRARY AND SST_RELEASE_LIBRARY)

  IF(SST_LIBRARIES)
    SET(SST_FOUND TRUE)
  ENDIF(SST_LIBRARIES)
ENDIF(SST_INCLUDE_DIRS)

IF(SST_FOUND)
  SET(SST_INCLUDE_DIRS ${SST_INCLUDE_DIRS}/sst) #sst includes are broken, so we need to use the include/sst directory
  IF(NOT SST_FIND_QUIETLY)
    MESSAGE(STATUS "Found SST: headers at ${SST_INCLUDE_DIRS}, libraries at ${SST_LIBRARY_DIRS}")
  ENDIF(NOT SST_FIND_QUIETLY)
ELSE(SST_FOUND)
  IF(SST_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "SST not found")
  ENDIF(SST_FIND_REQUIRED)
ENDIF(SST_FOUND)
