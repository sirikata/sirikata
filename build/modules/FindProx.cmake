# Searches for an installation of the Prox core library. On success, it sets the following variables:
#
#   PROX_FOUND              Set to true to indicate the proximity library was found
#   PROX_INCLUDE_DIRS       The directory containing the header file prox/QueryHandler.hpp
#   PROX_LIBRARIES          The libraries needed to use the library
#
# To specify an additional directory to search, set PROX_ROOT.
#
# Copyright (C) Siddhartha Chaudhuri, 2009
# Copyright (C) Ewen Cheslack-Postava, 2009
#

# Look for the header, first in the user-specified location and then in the system locations
SET(PROX_INCLUDE_DOC "The directory containing the header file prox/QueryHandler.hpp")
FIND_PATH(PROX_INCLUDE_DIRS NAMES prox/QueryHandler.hpp PATHS ${PROX_ROOT} ${PROX_ROOT}/include
          DOC ${PROX_INCLUDE_DOC} NO_DEFAULT_PATH)
IF(NOT PROX_INCLUDE_DIRS)  # now look in system locations
  FIND_PATH(PROX_INCLUDE_DIRS NAMES prox/QueryHandler.hpp DOC ${PROX_INCLUDE_DOC})
ENDIF(NOT PROX_INCLUDE_DIRS)

SET(PROX_FOUND FALSE)

IF(PROX_INCLUDE_DIRS)
  SET(PROX_LIBRARY_DIRS ${PROX_INCLUDE_DIRS})

  IF("${PROX_LIBRARY_DIRS}" MATCHES "/include$")
    # Strip off the trailing "/include" in the path.
    GET_FILENAME_COMPONENT(PROX_LIBRARY_DIRS ${PROX_LIBRARY_DIRS} PATH)
  ENDIF("${PROX_LIBRARY_DIRS}" MATCHES "/include$")

  IF(EXISTS "${PROX_LIBRARY_DIRS}/lib")
    SET(PROX_LIBRARY_DIRS ${PROX_LIBRARY_DIRS}/lib)
  ENDIF(EXISTS "${PROX_LIBRARY_DIRS}/lib")

  # Find PROX libraries
  FIND_LIBRARY(PROX_DEBUG_LIBRARY   NAMES prox_d proxd libprox_d libproxd
               PATH_SUFFIXES "" Debug   PATHS ${PROX_LIBRARY_DIRS} NO_DEFAULT_PATH)
  FIND_LIBRARY(PROX_RELEASE_LIBRARY NAMES prox libprox
               PATH_SUFFIXES "" Release PATHS ${PROX_LIBRARY_DIRS} NO_DEFAULT_PATH)

  SET(PROX_LIBRARIES )
  IF(PROX_DEBUG_LIBRARY AND PROX_RELEASE_LIBRARY)
    SET(PROX_LIBRARIES debug ${PROX_DEBUG_LIBRARY} optimized ${PROX_RELEASE_LIBRARY})
  ELSEIF(PROX_DEBUG_LIBRARY)
    SET(PROX_LIBRARIES ${PROX_DEBUG_LIBRARY})
  ELSEIF(PROX_RELEASE_LIBRARY)
    SET(PROX_LIBRARIES ${PROX_RELEASE_LIBRARY})
  ENDIF(PROX_DEBUG_LIBRARY AND PROX_RELEASE_LIBRARY)

  IF(PROX_LIBRARIES)
    SET(PROX_FOUND TRUE)
  ENDIF(PROX_LIBRARIES)
ENDIF(PROX_INCLUDE_DIRS)

IF(PROX_FOUND)
  IF(NOT PROX_FIND_QUIETLY)
    MESSAGE(STATUS "Found PROX: headers at ${PROX_INCLUDE_DIRS}, libraries at ${PROX_LIBRARY_DIRS}")
  ENDIF(NOT PROX_FIND_QUIETLY)
ELSE(PROX_FOUND)
  IF(PROX_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "PROX not found")
  ENDIF(PROX_FIND_REQUIRED)
ENDIF(PROX_FOUND)
