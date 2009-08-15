# Searches for an installation of the Sirikata core library. On success, it sets the following variables:
#
#   Enet_FOUND              Set to true to indicate the Sirikata core library was found
#   Enet_INCLUDE_DIRS       The directory containing the header file sirikata/util/Platform.hpp
#   Enet_LIBRARIES          The libraries needed to use the Sirikata core library
#
# To specify an additional directory to search, set Enet_ROOT.
#
# Copyright (C) Siddhartha Chaudhuri, 2009
#

# Look for the header, first in the user-specified location and then in the system locations
SET(Enet_INCLUDE_DOC "The directory containing the header file enet/enet.h")
FIND_PATH(Enet_INCLUDE_DIRS NAMES enet/enet.h PATHS ${Enet_ROOT} ${Enet_ROOT}/include
          DOC ${Enet_INCLUDE_DOC} NO_DEFAULT_PATH)
IF(NOT Enet_INCLUDE_DIRS)  # now look in system locations
  FIND_PATH(Enet_INCLUDE_DIRS NAMES enet/enet.h DOC ${Enet_INCLUDE_DOC})
ENDIF(NOT Enet_INCLUDE_DIRS)
SET(Enet_FOUND FALSE)

IF(Enet_INCLUDE_DIRS)
  SET(Enet_LIBRARY_DIRS ${Enet_INCLUDE_DIRS})

  IF("${Enet_LIBRARY_DIRS}" MATCHES "/include$")
    # Strip off the trailing "/include" in the path.
    GET_FILENAME_COMPONENT(Enet_LIBRARY_DIRS ${Enet_LIBRARY_DIRS} PATH)
  ENDIF("${Enet_LIBRARY_DIRS}" MATCHES "/include$")

  IF(EXISTS "${Enet_LIBRARY_DIRS}/lib")
    SET(Enet_LIBRARY_DIRS ${Enet_LIBRARY_DIRS}/lib)
  ENDIF(EXISTS "${Enet_LIBRARY_DIRS}/lib")

  # Find Enet libraries
  FIND_LIBRARY(Enet_DEBUG_LIBRARY   NAMES enet_d enetd libenet_d libenetd
               PATH_SUFFIXES "" Debug   PATHS ${Enet_LIBRARY_DIRS} NO_DEFAULT_PATH)
  FIND_LIBRARY(Enet_RELEASE_LIBRARY NAMES enet libenet
               PATH_SUFFIXES "" Release PATHS ${Enet_LIBRARY_DIRS} NO_DEFAULT_PATH)

  SET(Enet_LIBRARIES )
  IF(Enet_DEBUG_LIBRARY AND Enet_RELEASE_LIBRARY)
    SET(Enet_LIBRARIES debug ${Enet_DEBUG_LIBRARY} optimized ${Enet_RELEASE_LIBRARY})
  ELSEIF(Enet_DEBUG_LIBRARY)
    SET(Enet_LIBRARIES ${Enet_DEBUG_LIBRARY})
  ELSEIF(Enet_RELEASE_LIBRARY)
    SET(Enet_LIBRARIES ${Enet_RELEASE_LIBRARY})
  ENDIF(Enet_DEBUG_LIBRARY AND Enet_RELEASE_LIBRARY)

  IF(Enet_LIBRARIES)
    SET(Enet_FOUND TRUE)
  ENDIF(Enet_LIBRARIES)
ENDIF(Enet_INCLUDE_DIRS)

IF(Enet_FOUND)
  IF(NOT Enet_FIND_QUIETLY)
    MESSAGE(STATUS "Found Enet-Core: headers at ${Enet_INCLUDE_DIRS}, libraries at ${Enet_LIBRARY_DIRS}")
  ENDIF(NOT Enet_FIND_QUIETLY)
ELSE(Enet_FOUND)
  IF(Enet_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "Enet-Core not found")
  ENDIF(Enet_FIND_REQUIRED)
ENDIF(Enet_FOUND)
