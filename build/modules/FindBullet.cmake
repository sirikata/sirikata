# Searches for an bullet installation
#
# Defines:
#
#   bullet_FOUND          True if bullet was found, else false
#   bullet_LIBRARIES      Libraries to link (without full path)
#   bullet_LIBRARY_DIRS   Directories containing the libraries (-L option)
#   bullet_LDFLAGS        All required linker flags
#   bullet_INCLUDE_DIRS   The directories containing header files (-I option)
#   bullet_CFLAGS         All required compiler flags
#
# If you set bullet_MINIMUM_VERSION, only a version equal to or higher than this will be searched for. (This feature does not
# currently work on all systems.)
#
# On Windows, to specify the root directory of the bullet installation, set bullet_ROOT.
#
# Author: Siddhartha Chaudhuri, 2008
#

IF(WIN32)  # Windows

  SET(bullet_FOUND FALSE)

  IF(bullet_ROOT AND EXISTS "${bullet_ROOT}")
    SET(bullet_LIBRARY_DIRS ${bullet_ROOT}/lib)
    SET(bullet_LIBRARIES
        debug BulletCollision_d.lib BulletDynamics_d.lib BulletMultiThreaded_d.lib BulletSoftBody_d.lib LinearMath_d.lib
        optimized BulletCollision.lib BulletDynamics.lib BulletMultiThreaded.lib BulletSoftBody.lib LinearMath.lib)
    SET(bullet_LDFLAGS)
    IF(EXISTS ${bullet_ROOT}/include/bullet)
      SET(bullet_INCLUDE_DIRS ${bullet_ROOT}/include/bullet)
    ELSE()
      SET(bullet_INCLUDE_DIRS ${bullet_ROOT}/include)
    ENDIF()
    SET(bullet_CFLAGS)
    SET(bullet_FOUND TRUE)
  ENDIF(bullet_ROOT AND EXISTS "${bullet_ROOT}")

ELSEIF(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")  # OS X
#
  SET(bullet_FOUND FALSE)
#
#  #INCLUDE(CMakeFindFrameworks)
#  #CMAKE_FIND_FRAMEWORKS(bullet) 
#  SET(bullet_FRAMEWORKS ${bullet_ROOT}/Frameworks/bullet.framework)
#  IF(bullet_FRAMEWORKS)
#    LIST(GET bullet_FRAMEWORKS 0 bullet_LIBRARIES)
#    SET(bullet_INCLUDE_DIRS ${bullet_LIBRARIES}/Headers)
#    
#    # Unset other variables
#    SET(bullet_LDFLAGS -F${bullet_LIBRARIES}/.. -framework bullet)
#
#    SET(bullet_LIBRARY_DIRS)
#    SET(bullet_LIBRARIES)
#    SET(bullet_CFLAGS)
#    SET(bullet_FOUND TRUE)
#  ENDIF(bullet_FRAMEWORKS)

ELSE(WIN32)  # Linux etc
  FIND_PACKAGE(PkgConfig)
  IF(NOT PKG_CONFIG_FOUND)
    MESSAGE("Could not find pkg-config (to search for bullet)")
  ELSE(NOT PKG_CONFIG_FOUND)
    SET(ENV{PKG_CONFIG_PATH} ${bullet_ROOT}/lib/pkgconfig:${bullet_ROOT}/lib/pkgconfig)
    IF(bullet_MINIMUM_VERSION)
      PKG_CHECK_MODULES( bullet bullet>=${bullet_MINIMUM_VERSION})  # will set bullet_FOUND
    ELSE(bullet_MINIMUM_VERSION)
      PKG_CHECK_MODULES(bullet bullet)  # will set bullet_FOUND
    ENDIF(bullet_MINIMUM_VERSION)

  ENDIF(NOT PKG_CONFIG_FOUND)
  IF(bullet_FOUND)
  ENDIF()
ENDIF(WIN32)

IF(bullet_FOUND)
  IF(NOT bullet_FIND_QUIETLY)
    IF(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
      MESSAGE(STATUS "Found bullet: ${bullet_LDFLAGS}")
    ELSE(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
      MESSAGE(STATUS "Found bullet: headers at ${bullet_INCLUDE_DIRS}, libraries at ${bullet_LIBRARY_DIRS}")
    ENDIF(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
  ENDIF(NOT bullet_FIND_QUIETLY)
ELSE(bullet_FOUND)
  IF(bullet_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "bullet not found")
  ENDIF(bullet_FIND_REQUIRED)
ENDIF(bullet_FOUND)
#${TOP_LEVEL}/dependencies/bullet-2.74/src/.libs/libbulletcollision.so ${TOP_LEVEL}/dependencies/bullet-2.74/src/.libs/libbulletdynamics.so ${TOP_LEVEL}/dependencies/bullet-2.74/src/.libs/libbulletmath.so