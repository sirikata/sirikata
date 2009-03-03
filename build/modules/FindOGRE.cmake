# Searches for an OGRE installation
#
# Defines:
#
#   OGRE_FOUND          True if OGRE was found, else false
#   OGRE_LIBRARIES      Libraries to link (without full path)
#   OGRE_LIBRARY_DIRS   Directories containing the libraries (-L option)
#   OGRE_LDFLAGS        All required linker flags
#   OGRE_INCLUDE_DIRS   The directories containing header files (-I option)
#   OGRE_CFLAGS         All required compiler flags
#
# If you set OGRE_MINIMUM_VERSION, only a version equal to or higher than this will be searched for. (This feature does not
# currently work on all systems.)
#
# On Windows, to specify the root directory of the OGRE installation, set OGRE_ROOT.
#
# Author: Siddhartha Chaudhuri, 2008
#

IF(WIN32)  # Windows

  SET(OGRE_FOUND FALSE)

  IF(OGRE_ROOT AND EXISTS "${OGRE_ROOT}")
    SET(OGRE_LIBRARY_DIRS ${OGRE_ROOT}/lib)
    SET(OGRE_LIBRARIES
        debug OgreMain_d.lib
        optimized OgreMain.lib)
    SET(OGRE_LDFLAGS)
    SET(OGRE_INCLUDE_DIRS ${OGRE_ROOT}/include ${OGRE_ROOT}/samples/include)
    SET(OGRE_CFLAGS)

    SET(OGRE_FOUND TRUE)
  ENDIF(OGRE_ROOT AND EXISTS "${OGRE_ROOT}")

ELSEIF(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")  # OS X

  SET(OGRE_FOUND FALSE)

  #INCLUDE(CMakeFindFrameworks)
  #CMAKE_FIND_FRAMEWORKS(OGRE) 
  SET(OGRE_FRAMEWORKS ${OGRE_ROOT}/Frameworks/Ogre.framework)
  IF(OGRE_FRAMEWORKS)
    LIST(GET OGRE_FRAMEWORKS 0 OGRE_LIBRARIES)
    SET(OGRE_INCLUDE_DIRS ${OGRE_LIBRARIES}/Headers)
    
    # Unset other variables
    SET(OGRE_LDFLAGS -F${OGRE_LIBRARIES}/.. -framework Ogre)
    SET(OGRE_LIBRARY_DIRS)
    SET(OGRE_LIBRARIES)
    SET(OGRE_CFLAGS)

    SET(OGRE_FOUND TRUE)
  ENDIF(OGRE_FRAMEWORKS)

ELSE(WIN32)  # Linux etc

  FIND_PACKAGE(PkgConfig)
  IF(NOT PKG_CONFIG_FOUND)
    MESSAGE("Could not find pkg-config (to search for OGRE)")
  ELSE(NOT PKG_CONFIG_FOUND)
    IF(OGRE_MINIMUM_VERSION)
      PKG_CHECK_MODULES(OGRE OGRE >= ${OGRE_MINIMUM_VERSION})  # will set OGRE_FOUND
    ELSE(OGRE_MINIMUM_VERSION)
      PKG_CHECK_MODULES(OGRE OGRE)  # will set OGRE_FOUND
    ENDIF(OGRE_MINIMUM_VERSION)
  ENDIF(NOT PKG_CONFIG_FOUND)

ENDIF(WIN32)

IF(OGRE_FOUND)
  IF(NOT OGRE_FIND_QUIETLY)
    IF(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
      MESSAGE(STATUS "Found OGRE: ${OGRE_LDFLAGS}")
    ELSE(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
      MESSAGE(STATUS "Found OGRE: headers at ${OGRE_INCLUDE_DIRS}, libraries at ${OGRE_LIBRARY_DIRS}")
    ENDIF(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
  ENDIF(NOT OGRE_FIND_QUIETLY)
ELSE(OGRE_FOUND)
  IF(OGRE_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "OGRE not found")
  ENDIF(OGRE_FIND_REQUIRED)
ENDIF(OGRE_FOUND)
