# Searches for an installation of the Sirikata core library. On success, it sets the following variables:
#
#   Raknet_FOUND              Set to true to indicate the Sirikata core library was found
#   Raknet_INCLUDE_DIRS       The directory containing the header file sirikata/util/Platform.hpp
#   Raknet_LIBRARIES          The libraries needed to use the Sirikata core library
#
# To specify an additional directory to search, set Raknet_ROOT.
#
# Copyright (C) Siddhartha Chaudhuri, 2009
#

# Look for the header, first in the user-specified location and then in the system locations
SET(Raknet_INCLUDE_DOC "The directory containing the header file raknet/RakNetworkFactory.h")
FIND_PATH(Raknet_INCLUDE_DIRS NAMES raknet/RakNetworkFactory.h PATHS ${Raknet_ROOT} ${Raknet_ROOT}/include
          DOC ${Raknet_INCLUDE_DOC} NO_DEFAULT_PATH)
IF(NOT Raknet_INCLUDE_DIRS)  # now look in system locations
  FIND_PATH(Raknet_INCLUDE_DIRS NAMES raknet/RakNetworkFactor.h DOC ${Raknet_INCLUDE_DOC})
ENDIF(NOT Raknet_INCLUDE_DIRS)

SET(Raknet_FOUND FALSE)

IF(Raknet_INCLUDE_DIRS)
  SET(Raknet_LIBRARY_DIRS ${Raknet_INCLUDE_DIRS})

  IF("${Raknet_LIBRARY_DIRS}" MATCHES "/include$")
    # Strip off the trailing "/include" in the path.
    GET_FILENAME_COMPONENT(Raknet_LIBRARY_DIRS ${Raknet_LIBRARY_DIRS} PATH)
  ENDIF("${Raknet_LIBRARY_DIRS}" MATCHES "/include$")

  IF(EXISTS "${Raknet_LIBRARY_DIRS}/lib")
    SET(Raknet_LIBRARY_DIRS ${Raknet_LIBRARY_DIRS}/lib)
  ENDIF(EXISTS "${Raknet_LIBRARY_DIRS}/lib")

  # Find Raknet libraries
  FIND_LIBRARY(Raknet_DEBUG_LIBRARY   NAMES raknet_d raknetd libraknet_d libraknetd
               PATH_SUFFIXES "" Debug   PATHS ${Raknet_LIBRARY_DIRS} NO_DEFAULT_PATH)
  FIND_LIBRARY(Raknet_RELEASE_LIBRARY NAMES raknet libraknet
               PATH_SUFFIXES "" Release PATHS ${Raknet_LIBRARY_DIRS} NO_DEFAULT_PATH)

  SET(Raknet_LIBRARIES )
  IF(Raknet_DEBUG_LIBRARY AND Raknet_RELEASE_LIBRARY)
    SET(Raknet_LIBRARIES debug ${Raknet_DEBUG_LIBRARY} optimized ${Raknet_RELEASE_LIBRARY})
  ELSEIF(Raknet_DEBUG_LIBRARY)
    SET(Raknet_LIBRARIES ${Raknet_DEBUG_LIBRARY})
  ELSEIF(Raknet_RELEASE_LIBRARY)
    SET(Raknet_LIBRARIES ${Raknet_RELEASE_LIBRARY})
  ENDIF(Raknet_DEBUG_LIBRARY AND Raknet_RELEASE_LIBRARY)

  IF(Raknet_LIBRARIES)
    SET(Raknet_FOUND TRUE)
  ENDIF(Raknet_LIBRARIES)
ENDIF(Raknet_INCLUDE_DIRS)

IF(Raknet_FOUND)
  IF(NOT Raknet_FIND_QUIETLY)
    MESSAGE(STATUS "Found Raknet-Core: headers at ${Raknet_INCLUDE_DIRS}, libraries at ${Raknet_LIBRARY_DIRS}")
  ENDIF(NOT Raknet_FIND_QUIETLY)
ELSE(Raknet_FOUND)
  IF(Raknet_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "Raknet-Core not found")
  ENDIF(Raknet_FIND_REQUIRED)
ENDIF(Raknet_FOUND)
