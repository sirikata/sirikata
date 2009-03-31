# Searches for an sdl installation
#
# Defines:
#
#   sdl_FOUND          True if sdl was found, else false
#   sdl_LIBRARIES      Libraries to link (without full path)
#   sdl_LIBRARY_DIRS   Directories containing the libraries (-L option)
#   sdl_LDFLAGS        All required linker flags
#   sdl_INCLUDE_DIRS   The directories containing header files (-I option)
#   sdl_CFLAGS         All required compiler flags
#
# If you set sdl_MINIMUM_VERSION, only a version equal to or higher than this will be searched for. (This feature does not
# currently work on all systems.)
#
# On Windows, to specify the root directory of the sdl installation, set sdl_ROOT.
#
# Author: Siddhartha Chaudhuri, 2008
#

IF(WIN32)  # Windows

  SET(sdl_FOUND FALSE)

  IF(sdl_ROOT AND EXISTS "${sdl_ROOT}")
    SET(sdl_LIBRARY_DIRS ${sdl_ROOT}/lib)
    SET(sdl_LIBRARIES
        debug sdl_d.lib
        optimized sdl.lib)
    SET(sdl_LDFLAGS)
    SET(sdl_INCLUDE_DIRS ${sdl_ROOT}/include ${sdl_ROOT}/samples/include)
    SET(sdl_CFLAGS)

    SET(sdl_FOUND TRUE)

  ENDIF(sdl_ROOT AND EXISTS "${sdl_ROOT}")

ELSEIF(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")  # OS X

  SET(sdl_FOUND FALSE)

  #INCLUDE(CMakeFindFrameworks)
  #CMAKE_FIND_FRAMEWORKS(sdl) 
  SET(sdl_FRAMEWORKS ${sdl_ROOT}/Frameworks/SDL.framework)
  IF(sdl_FRAMEWORKS)
    LIST(GET sdl_FRAMEWORKS 0 sdl_LIBRARIES)
    SET(sdl_INCLUDE_DIRS ${sdl_LIBRARIES}/Headers)
    
    # Unset other variables
    SET(sdl_LDFLAGS -F${sdl_LIBRARIES}/.. -framework SDL)

    SET(sdl_LIBRARY_DIRS)
    SET(sdl_LIBRARIES)
    SET(sdl_CFLAGS)
    SET(sdl_FOUND TRUE)
  ENDIF(sdl_FRAMEWORKS)

ELSE(WIN32)  # Linux etc
  FIND_PACKAGE(PkgConfig)
  IF(NOT PKG_CONFIG_FOUND)
    MESSAGE("Could not find pkg-config (to search for sdl)")
  ELSE(NOT PKG_CONFIG_FOUND)
    SET(ENV{PKG_CONFIG_PATH} ${sdl_ROOT}/../lib/pkgconfig:${sdl_ROOT}/lib/pkgconfig)
    IF(sdl_MINIMUM_VERSION)
      PKG_CHECK_MODULES( sdl sdl>=${sdl_MINIMUM_VERSION})  # will set sdl_FOUND
    ELSE(sdl_MINIMUM_VERSION)
      PKG_CHECK_MODULES(sdl sdl)  # will set sdl_FOUND
    ENDIF(sdl_MINIMUM_VERSION)

  ENDIF(NOT PKG_CONFIG_FOUND)
  IF(sdl_FOUND)
  ENDIF()
ENDIF(WIN32)

IF(sdl_FOUND)
  IF(NOT sdl_FIND_QUIETLY)
    IF(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
      MESSAGE(STATUS "Found sdl: ${sdl_LDFLAGS}")
    ELSE(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
      MESSAGE(STATUS "Found sdl: headers at ${sdl_INCLUDE_DIRS}, libraries at ${sdl_LIBRARY_DIRS}")
    ENDIF(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
  ENDIF(NOT sdl_FIND_QUIETLY)
ELSE(sdl_FOUND)
  IF(sdl_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "sdl not found")
  ENDIF(sdl_FIND_REQUIRED)
ENDIF(sdl_FOUND)
